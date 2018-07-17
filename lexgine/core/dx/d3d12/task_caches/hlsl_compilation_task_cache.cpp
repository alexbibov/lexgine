#include "hlsl_compilation_task_cache.h"

#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/misc/datetime.h"
#include "lexgine/core/misc/optional.h"
#include "lexgine/core/misc/misc.h"
#include "lexgine/core/misc/strict_weak_ordering.h"
#include "lexgine/core/dx/d3d12/tasks/hlsl_compilation_task.h"
#include "combined_cache_key.h"


using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::task_caches;
using namespace lexgine::core::dx::d3d12::tasks;


namespace {

uint64_t hashStringifiedDefines(std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions)
{
    std::string rv{};
    {
        rv += "DEFINES={";
        for (auto const& def : macro_definitions)
            rv += "{NAME=" + def.name + ", VALUE=" + def.value + "}, ";
        rv += "}";
    }

    return misc::HashedString{ rv }.hash();
}

}


std::string HLSLCompilationTaskCache::Key::toString() const
{
    // Note: do not change, the returned string is conventional

    return "{" + std::string{ source_path } +"}__{"
        + std::to_string(hash_value) + "__{"
        + HLSLCompilationTask::shaderModelAndTypeToTargetName(
            static_cast<dxcompilation::ShaderModel>(shader_model),
            static_cast<dxcompilation::ShaderType>(shader_type));
}

void HLSLCompilationTaskCache::Key::serialize(void* p_serialization_blob) const
{
    uint8_t* ptr{ static_cast<uint8_t*>(p_serialization_blob) };

    strcpy_s(reinterpret_cast<char*>(ptr), max_string_section_length_in_bytes, source_path); ptr += max_string_section_length_in_bytes;
    
    memcpy(ptr, &shader_type, sizeof(shader_type)); ptr += sizeof(shader_type);
    memcpy(ptr, &shader_model, sizeof(shader_model)); ptr += sizeof(shader_model);
    memcpy(ptr, &hash_value, sizeof(hash_value));
}

void HLSLCompilationTaskCache::Key::deserialize(void const* p_serialization_blob)
{
    uint8_t const* ptr{ static_cast<uint8_t const*>(p_serialization_blob) };

    strcpy_s(source_path, max_string_section_length_in_bytes, reinterpret_cast<char const*>(ptr)); ptr += max_string_section_length_in_bytes;
    
    memcpy(&shader_type, ptr, sizeof(shader_type)); ptr += sizeof(shader_type);
    memcpy(&shader_model, ptr, sizeof(shader_model)); ptr += sizeof(shader_model);
    memcpy(&hash_value, ptr, sizeof(hash_value));
}

HLSLCompilationTaskCache::Key::Key(std::string const& hlsl_source_path,
    uint16_t shader_type,
    uint16_t shader_model,
    uint64_t hash_value) :
    shader_type{ shader_type },
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


tasks::HLSLCompilationTask* HLSLCompilationTaskCache::addTask(core::Globals& globals, std::string const& source, std::string const& source_name, 
    dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point, 
    ShaderSourceCodePreprocessor::SourceType source_type,
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions, 
    dxcompilation::HLSLCompilationOptimizationLevel optimization_level, 
    bool strict_mode, bool force_all_resources_be_bound, 
    bool force_ieee_standard, bool treat_warnings_as_errors, bool enable_validation,
    bool enable_debug_information, bool enable_16bit_types)
{
    uint64_t hash_value = hashStringifiedDefines(macro_definitions);

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
            for (auto const& path_prefix : global_settings.getShaderLookupDirectories())
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
        hlsl_source_code = ShaderSourceCodePreprocessor{ path_to_shader, source_type }.getPreprocessedSource();
        timestamp = misc::getFileLastUpdatedTimeStamp(path_to_shader);

        // Generate hash value
        key = Key{ path_to_shader, 
            static_cast<unsigned short>(shader_type),
            static_cast<unsigned short>(shader_model), 
            hash_value };
    }
    else
    {
        hlsl_source_code = source;
        timestamp = misc::DateTime::now();    // NOTE: HLSL sources supplied directly (i.e. not via source files) are always recompiled
        key = Key{ source_name,
            static_cast<unsigned short>(shader_type),
            static_cast<unsigned short>(shader_model),
            hash_value };
    }
    CombinedCacheKey combined_key{ key };

    
    HLSLCompilationTask* inserted_task_ptr{ nullptr };

    if (m_tasks_cache_keys.find(combined_key) == m_tasks_cache_keys.end())
    {
        auto cache_map_insertion_position = 
            m_tasks_cache_keys.insert(std::make_pair(combined_key, cache_storage::iterator{})).first;

        m_tasks.emplace_back(
            cache_map_insertion_position->first,
            timestamp.isValid() ? static_cast<misc::DateTime>(timestamp) : misc::DateTime::now(),
            globals, hlsl_source_code, source_name, shader_model, shader_type, shader_entry_point,
            macro_definitions, optimization_level,
            strict_mode, force_all_resources_be_bound,
            force_ieee_standard, treat_warnings_as_errors,
            enable_validation, enable_debug_information, enable_16bit_types);

        cache_storage::iterator p = --m_tasks.end();
        cache_map_insertion_position->second = p;
        HLSLCompilationTask& new_hlsl_compilation_task_ref = *p;
        inserted_task_ptr = &new_hlsl_compilation_task_ref;
    }
    else if (source_type == ShaderSourceCodePreprocessor::SourceType::string)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "HLSL source code with key \"" + key.toString() + "\" already exists in the cache. "
            "In order to circumvent this issue make sure that all HLSL sources that are directly (i.e. not via files) supplied for "
            "compilation are having unique source names");
    }

    if (!globals.get<GlobalSettings>()->isDeferredShaderCompilationOn())
    {
        inserted_task_ptr->execute(0);
    }

    return inserted_task_ptr;
}

HLSLCompilationTaskCache::cache_storage& task_caches::HLSLCompilationTaskCache::storage()
{
    return m_tasks;
}

HLSLCompilationTaskCache::cache_storage const& task_caches::HLSLCompilationTaskCache::storage() const
{
    return m_tasks;
}