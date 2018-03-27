#include "hlsl_compilation_task_cache.h"

#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/misc/datetime.h"
#include "lexgine/core/misc/optional.h"
#include "lexgine/core/misc/misc.h"
#include "lexgine/core/misc/strict_weak_ordering.h"
#include "lexgine/core/dx/d3d12/tasks/hlsl_compilation_task.h"


using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::task_caches;
using namespace lexgine::core::dx::d3d12::tasks;




std::string HLSLCompilationTaskCache::Key::toString() const
{
    return std::string{ std::string{ "path:{" } +source_path + "}__hash:{" + std::to_string(hash_value) + "}" };
}

void HLSLCompilationTaskCache::Key::serialize(void* p_serialization_blob) const
{
    uint8_t* ptr{ static_cast<uint8_t*>(p_serialization_blob) };

    memcpy(ptr, source_path, sizeof(source_path)); ptr += sizeof(source_path);
    memcpy(ptr, &shader_model, sizeof(shader_model)); ptr += sizeof(shader_model);
    memcpy(ptr, &hash_value, sizeof(hash_value));
}

void HLSLCompilationTaskCache::Key::deserialize(void const* p_serialization_blob)
{
    uint8_t const* ptr{ static_cast<uint8_t const*>(p_serialization_blob) };

    memcpy(source_path, ptr, sizeof(source_path)); ptr += sizeof(source_path);
    memcpy(&shader_model, ptr, sizeof(shader_model)); ptr += sizeof(shader_model);
    memcpy(&hash_value, ptr, sizeof(hash_value));
}

HLSLCompilationTaskCache::Key::Key(std::string const& hlsl_source_path,
    uint16_t shader_model,
    uint64_t hash_value) :
    shader_model{ shader_model },
    hash_value{ hash_value }
{
    memset(source_path, 0, max_string_section_length_in_bytes);
    memcpy(source_path, hlsl_source_path.c_str(), max_string_section_length_in_bytes);
}

bool HLSLCompilationTaskCache::Key::operator<(Key const& other) const
{
    int r = std::strcmp(source_path, other.source_path);
    SWO_STEP(r, < , 0);
    SWO_STEP(shader_model, < , other.shader_model);
    SWO_END(hash_value, < , other.hash_value);
}

bool HLSLCompilationTaskCache::Key::operator==(Key const& other) const
{
    return strcmp(source_path, other.source_path) == 0
        && shader_model == other.shader_model
        && hash_value == other.hash_value;
}


HLSLCompilationTaskCache::HLSLCompilationTaskCache()
{
}

void HLSLCompilationTaskCache::addTask(core::Globals& globals, std::string const& source, std::string const& source_name, 
    dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point, 
    ShaderSourceCodePreprocessor::SourceType source_type,
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions, 
    dxcompilation::HLSLCompilationOptimizationLevel optimization_level, 
    bool strict_mode, bool force_all_resources_be_bound, 
    bool force_ieee_standard, bool treat_warnings_as_errors, bool enable_validation,
    bool enable_debug_information, bool enable_16bit_types)
{
    std::string stringified_defines{};
    {
        stringified_defines += "DEFINES={";
        for (auto const& def : macro_definitions)
            stringified_defines += "{NAME=" + def.name + ", VALUE=" + def.value + "}, ";
        stringified_defines += "}";
    }

    std::string hlsl_source_code{};
    misc::Optional<misc::DateTime> timestamp{};
    Key key{};

    if(source_type == ShaderSourceCodePreprocessor::SourceType::file)
    {
        auto& global_settings = *globals.get<GlobalSettings>();

        // find the full correct path to the shader if the shader exists
        std::string path_to_shader{};
        {
            bool shader_found{ false };
            for (auto path_prefix : global_settings.getShaderLookupDirectories())
            {
                path_to_shader = path_prefix + source;
                if (misc::doesFileExist(path_to_shader))
                {
                    shader_found = true;
                    break;
                }
            }

            if (!shader_found)
            {
                LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Unable to retrieve shader asset \"" + source + "\"");
            }
        }

        // Retrieve shader source code and time stamp
        hlsl_source_code = ShaderSourceCodePreprocessor{ source, source_type }.getPreprocessedSource();
        timestamp = misc::getFileLastUpdatedTimeStamp(path_to_shader);

        // Generate hash value
        key = Key{ path_to_shader, 
            static_cast<unsigned short>(shader_model), 
            misc::HashedString{ stringified_defines }.hash() };
    }
    else
    {
        hlsl_source_code = source;
        timestamp = misc::DateTime::now();    // NOTE: HLSL sources supplied directly (i.e. not via source files) are always recompiled
        key = Key{ hlsl_source_code,
        static_cast<unsigned short>(shader_model),
        misc::HashedString{hlsl_source_code + "__" + stringified_defines}.hash() };
    }


    if (m_tasks_cache_keys.find(key) == m_tasks_cache_keys.end())
    {
        m_tasks.emplace_back(
            key, timestamp.isValid() ? static_cast<misc::DateTime>(timestamp) : misc::DateTime::now(),
            globals, hlsl_source_code, source_name, shader_model, shader_type, shader_entry_point,
            macro_definitions, optimization_level,
            strict_mode, force_all_resources_be_bound,
            force_ieee_standard, treat_warnings_as_errors,
            enable_validation, enable_debug_information, enable_16bit_types);

        m_tasks_cache_keys.insert(std::make_pair(key, --m_tasks.end()));
    }
}

HLSLCompilationTaskCache::iterator HLSLCompilationTaskCache::begin()
{
    return m_tasks.begin();
}

HLSLCompilationTaskCache::iterator HLSLCompilationTaskCache::end()
{
    return m_tasks.end();
}

HLSLCompilationTaskCache::const_iterator HLSLCompilationTaskCache::begin() const
{
    return m_tasks.begin();
}

HLSLCompilationTaskCache::const_iterator HLSLCompilationTaskCache::end() const
{
    return m_tasks.end();
}

size_t HLSLCompilationTaskCache::size() const
{
    return m_tasks.size();
}

tasks::HLSLCompilationTask* task_caches::HLSLCompilationTaskCache::find(Key const& key)
{
    HLSLCompilationTask* rv{ nullptr };
    cache_mapping::iterator p;
    if ((p = m_tasks_cache_keys.find(key)) != m_tasks_cache_keys.end())
    {
        HLSLCompilationTask& temp = (*p->second);
        rv = &temp;
    }

    return rv;
}

tasks::HLSLCompilationTask const* task_caches::HLSLCompilationTaskCache::find(Key const& key) const
{
    HLSLCompilationTask const* rv{ nullptr };
    cache_mapping::const_iterator p;
    if ((p = m_tasks_cache_keys.find(key)) != m_tasks_cache_keys.end())
    {
        HLSLCompilationTask const& temp = (*p->second);
        rv = &temp;
    }

    return rv;
}


