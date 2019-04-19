#ifndef LEXGINE_CORE_DX_D3D12_TASK_CACHES_HLSL_COMPILATION_TASK_CACHE_H
#define LEXGINE_CORE_DX_D3D12_TASK_CACHES_HLSL_COMPILATION_TASK_CACHE_H

#include <list>
#include <map>

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/shader_source_code_preprocessor.h"
#include "lexgine/core/dx/dxcompilation/common.h"
#include "lexgine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "lexgine/core/misc/datetime.h"


namespace lexgine::core::dx::d3d12::task_caches {

class HLSLTranslationUnit
{
public:
    std::string const& name() const { return m_source_name; }
    std::string const& source() const { return m_hlsl_source_code; }
    misc::DateTime const& timestamp() const { return m_timestamp; }

    core::Globals& globals() const { return m_globals; }

protected:
    HLSLTranslationUnit(Globals& globals)
        : m_globals{ globals } {}

protected:
    core::Globals& m_globals;
    std::string m_source_name;
    std::string m_hlsl_source_code;
    misc::DateTime m_timestamp;
};

class HLSLFileTranslationUnit final : public HLSLTranslationUnit
{
public:
    HLSLFileTranslationUnit(Globals& globals, std::string const& source_name, std::string const& file_path);

    std::string const& pathToShader() const { return m_path_to_shader; }

private:
    std::string m_path_to_shader;
};

class HLSLSourceTranslationUnit final : public HLSLTranslationUnit
{
public:
    HLSLSourceTranslationUnit(Globals& globals, std::string const& source_name, std::string const& hlsl_source_code)
        : HLSLTranslationUnit{ globals }
    {
        m_source_name = source_name;
        m_hlsl_source_code = ShaderSourceCodePreprocessor{ hlsl_source_code, ShaderSourceCodePreprocessor::SourceType::string }.getPreprocessedSource();
        m_timestamp = misc::DateTime::buildTime();
    }
};


//! Dictionary of HLSL compilation tasks
class HLSLCompilationTaskCache : public NamedEntity<class_names::D3D12_HLSLCompilationTaskCache>
{
    friend class tasks::HLSLCompilationTask;
    friend class CombinedCacheKey;

public:

    using cache_storage = std::list<tasks::HLSLCompilationTask>;

private:
    struct Key final
    {
        static constexpr size_t max_string_section_length_in_bytes = 1024U;

        char source_path[max_string_section_length_in_bytes];
        uint16_t shader_type;
        uint16_t shader_model;
        uint64_t hash_value;


        static constexpr size_t const serialized_size =
            max_string_section_length_in_bytes
            + sizeof(uint16_t)
            + sizeof(uint16_t)
            + sizeof(uint64_t);


        std::string toString() const;

        void serialize(void* p_serialization_blob) const;
        void deserialize(void const* p_serialization_blob);

        Key(std::string const& hlsl_source_path,
            uint16_t shader_type,
            uint16_t shader_model,
            uint64_t hash_value);
        Key() = default;

        bool operator<(Key const& other) const;
        bool operator==(Key const& other) const;
    };

    using cache_mapping = std::map<CombinedCacheKey, cache_storage::iterator>;

public:
    HLSLCompilationTaskCache();
    ~HLSLCompilationTaskCache();

    tasks::HLSLCompilationTask* findOrCreateTask(HLSLFileTranslationUnit const& hlsl_translation_unit,
        dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
        std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions = std::list<dxcompilation::HLSLMacroDefinition>{},
        dxcompilation::HLSLCompilationOptimizationLevel optimization_level = dxcompilation::HLSLCompilationOptimizationLevel::level3,
        bool strict_mode = true, bool force_all_resources_be_bound = false,
        bool force_ieee_standart = true, bool treat_warnings_as_errors = true, bool enable_validation = true,
        bool enable_debug_information = false, bool enable_16bit_types = false);

    tasks::HLSLCompilationTask* findOrCreateTask(HLSLSourceTranslationUnit const& hlsl_translation_unit,
        dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
        std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions = std::list<dxcompilation::HLSLMacroDefinition>{},
        dxcompilation::HLSLCompilationOptimizationLevel optimization_level = dxcompilation::HLSLCompilationOptimizationLevel::level3,
        bool strict_mode = true, bool force_all_resources_be_bound = false,
        bool force_ieee_standart = true, bool treat_warnings_as_errors = true, bool enable_validation = true,
        bool enable_debug_information = false, bool enable_16bit_types = false);

    
    //! Returns reference to internal cache storage containing HLSL compilation tasks
    cache_storage& storage();

    //! Returns constant reference to internal cache storage containing HLSL compilation tasks
    cache_storage const& storage() const;

private:
    class impl;

private:
    std::unique_ptr<impl> m_impl;
    cache_storage m_tasks;
    cache_mapping m_tasks_cache_keys;
};

}

#endif