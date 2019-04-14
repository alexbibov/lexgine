#include "hlsl_compilation_task_cache.h"

#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/exception.h"
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

std::string getStringifiedDefines(std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions)
{
    std::string rv{};
    {
        rv += "DEFINES={";
        for (auto const& def : macro_definitions)
            rv += "{NAME=" + def.name + ", VALUE=" + def.value + "}, ";
        rv += "}";
    }

    return rv;
}

}


HLSLFileTranslationUnit::HLSLFileTranslationUnit(Globals& globals, std::string const& source_name, std::string const& file_path)
    : HLSLTranslationUnit{ globals }
{
    m_source_name = source_name;

    {
        auto& global_settings = *globals.get<GlobalSettings>();

        // find the full correct path to the shader if the shader exists
        
        {
            bool shader_found{ false };
            for (auto const& path_prefix : global_settings.getShaderLookupDirectories())
            {
                m_path_to_shader = path_prefix + file_path;
                if (misc::doesFileExist(m_path_to_shader))
                {
                    shader_found = true;
                    break;
                }
            }

            if (!shader_found)
            {
                LEXGINE_THROW_ERROR("Unable to retrieve shader asset \"" + file_path + "\"");
            }
        }

        m_hlsl_source_code = ShaderSourceCodePreprocessor{ m_path_to_shader, ShaderSourceCodePreprocessor::SourceType::file }.getPreprocessedSource();
    }

    m_timestamp = misc::getFileLastUpdatedTimeStamp(m_path_to_shader);
}


class HLSLCompilationTaskCache::impl
{
public:
    impl(HLSLCompilationTaskCache& enclosing)
        : m_enclosing{ enclosing }
    {

    }

    HLSLCompilationTask* insertHLSLCompilationTask(Globals& globals, CombinedCacheKey const& key,
        misc::DateTime const& timestamp, std::string const& processed_hlsl_source_code,
        std::string const& source_name, dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type,
        std::string const& shader_entry_point, std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions, dxcompilation::HLSLCompilationOptimizationLevel optimization_level,
        bool strict_mode, bool force_all_resources_be_bound, bool force_ieee_standart,
        bool treat_warnings_as_errors, bool enable_validation, bool enable_debug_information, bool enable_16bit_types)
    {
        HLSLCompilationTask* inserted_task_ptr{ nullptr };
        auto q = m_enclosing.m_tasks_cache_keys.find(key);
        if (q == m_enclosing.m_tasks_cache_keys.end())
        {
            auto cache_map_insertion_position =
                m_enclosing.m_tasks_cache_keys.insert(std::make_pair(key, cache_storage::iterator{})).first;

            m_enclosing.m_tasks.emplace_back(
                cache_map_insertion_position->first,
                timestamp,
                globals, processed_hlsl_source_code, source_name, shader_model, shader_type, shader_entry_point,
                macro_definitions, optimization_level,
                strict_mode, force_all_resources_be_bound,
                force_ieee_standart, treat_warnings_as_errors,
                enable_validation, enable_debug_information, enable_16bit_types);

            cache_storage::iterator p = --m_enclosing.m_tasks.end();
            cache_map_insertion_position->second = p;
            HLSLCompilationTask& new_hlsl_compilation_task_ref = *p;
            inserted_task_ptr = &new_hlsl_compilation_task_ref;
        }
        else
        {
            return &(*q->second);
        }

        if (!globals.get<GlobalSettings>()->isDeferredShaderCompilationOn())
        {
            inserted_task_ptr->execute(0);
        }

        return inserted_task_ptr;
    }

private:
    HLSLCompilationTaskCache& m_enclosing;
};


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

    memcpy(ptr, &shader_type, sizeof(uint16_t)); ptr += sizeof(uint16_t);
    memcpy(ptr, &shader_model, sizeof(uint16_t)); ptr += sizeof(uint16_t);
    memcpy(ptr, &hash_value, sizeof(uint64_t));
}

void HLSLCompilationTaskCache::Key::deserialize(void const* p_serialization_blob)
{
    uint8_t const* ptr{ static_cast<uint8_t const*>(p_serialization_blob) };

    strcpy_s(source_path, max_string_section_length_in_bytes, reinterpret_cast<char const*>(ptr)); ptr += max_string_section_length_in_bytes;

    memcpy(&shader_type, ptr, sizeof(uint16_t)); ptr += sizeof(uint16_t);
    memcpy(&shader_model, ptr, sizeof(uint16_t)); ptr += sizeof(uint16_t);
    memcpy(&hash_value, ptr, sizeof(uint64_t));
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

HLSLCompilationTaskCache::HLSLCompilationTaskCache()
    : m_impl{ new impl{*this} }
{

}

HLSLCompilationTaskCache::~HLSLCompilationTaskCache() = default;

tasks::HLSLCompilationTask* HLSLCompilationTaskCache::findOrCreateTask(HLSLFileTranslationUnit const& hlsl_translation_unit,
    dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions,
    dxcompilation::HLSLCompilationOptimizationLevel optimization_level,
    bool strict_mode, bool force_all_resources_be_bound,
    bool force_ieee_standart, bool treat_warnings_as_errors, bool enable_validation,
    bool enable_debug_information, bool enable_16bit_types)
{
    uint64_t hash_value = misc::HashedString{ getStringifiedDefines(macro_definitions) + hlsl_translation_unit.source() }.hash();

    Key key{ hlsl_translation_unit.pathToShader(),
        static_cast<unsigned short>(shader_type),
        static_cast<unsigned short>(shader_model),
        hash_value };
    CombinedCacheKey combined_key{ key };

    return m_impl->insertHLSLCompilationTask(hlsl_translation_unit.globals(), combined_key, hlsl_translation_unit.timestamp(),
        hlsl_translation_unit.source(), hlsl_translation_unit.name(), shader_model, shader_type, shader_entry_point,
        macro_definitions, optimization_level, strict_mode, force_all_resources_be_bound, force_ieee_standart, treat_warnings_as_errors,
        enable_validation, enable_debug_information, enable_16bit_types);
}

tasks::HLSLCompilationTask* HLSLCompilationTaskCache::findOrCreateTask(HLSLSourceTranslationUnit const& hlsl_translation_unit,
    dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions,
    dxcompilation::HLSLCompilationOptimizationLevel optimization_level,
    bool strict_mode, bool force_all_resources_be_bound,
    bool force_ieee_standart, bool treat_warnings_as_errors, bool enable_validation,
    bool enable_debug_information, bool enable_16bit_types)
{
    uint64_t hash_value = misc::HashedString{ getStringifiedDefines(macro_definitions) + hlsl_translation_unit.source() }.hash();

    Key key{ hlsl_translation_unit.name(),
        static_cast<unsigned short>(shader_type),
        static_cast<unsigned short>(shader_model),
        hash_value };
    CombinedCacheKey combined_key{ key };

    return m_impl->insertHLSLCompilationTask(hlsl_translation_unit.globals(), combined_key, hlsl_translation_unit.timestamp(),
        hlsl_translation_unit.source(), hlsl_translation_unit.name(), shader_model, shader_type, shader_entry_point,
        macro_definitions, optimization_level, strict_mode, force_all_resources_be_bound, force_ieee_standart, treat_warnings_as_errors,
        enable_validation, enable_debug_information, enable_16bit_types);
}

HLSLCompilationTaskCache::cache_storage& task_caches::HLSLCompilationTaskCache::storage()
{
    return m_tasks;
}

HLSLCompilationTaskCache::cache_storage const& task_caches::HLSLCompilationTaskCache::storage() const
{
    return m_tasks;
}
