#ifndef LEXGINE_CORE_DX_D3D12_TASK_CACHES_HLSL_COMPILATION_TASK_CACHE_H

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/shader_source_code_preprocessor.h"
#include "lexgine/core/dx/dxcompilation/common.h"
#include "lexgine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"


#include <list>
#include <map>


namespace lexgine {namespace core {namespace dx {namespace d3d12 {namespace task_caches {

//! Dictionary of HLSL compilation tasks
class HLSLCompilationTaskCache : public NamedEntity<class_names::HLSLCompilationTaskCache>
{
public:

    using cache_storage = std::list<tasks::HLSLCompilationTask>;

    struct Key final
    {
        static constexpr size_t max_string_section_length_in_bytes = 512U;

        char source_path[max_string_section_length_in_bytes];
        uint16_t shader_model;
        uint64_t hash_value;


        static size_t const serialized_size =
            max_string_section_length_in_bytes
            + sizeof(uint16_t)
            + sizeof(uint64_t);


        std::string toString() const;

        void serialize(void* p_serialization_blob) const;
        void deserialize(void const* p_serialization_blob);

        Key(std::string const& hlsl_source_path,
            uint16_t shader_model,
            uint64_t hash_value);
        Key() = default;

        bool operator<(Key const& other) const;
        bool operator==(Key const& other) const;
    };

    using cache_mapping = std::map<Key, cache_storage::iterator>;

public:

    HLSLCompilationTaskCache();

    void addTask(core::Globals& globals, std::string const& source, std::string const& source_name,
        dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
        ShaderSourceCodePreprocessor::SourceType source_type,
        std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions = std::list<dxcompilation::HLSLMacroDefinition>{},
        dxcompilation::HLSLCompilationOptimizationLevel optimization_level = dxcompilation::HLSLCompilationOptimizationLevel::level3,
        bool strict_mode = true, bool force_all_resources_be_bound = false,
        bool force_ieee_standard = true, bool treat_warnings_as_errors = true, bool enable_validation = true,
        bool enable_debug_information = false, bool enable_16bit_types = false);

    
    //! Returns reference to internal cache storage containing HLSL compilation tasks
    cache_storage& storage();

    //! Returns constant reference to internal cache storage containing HLSL compilation tasks
    cache_storage const& storage() const;


    /*! attempts to locate HLSL compilation task with provided key within the cache.
     If the search was not successful returns nullptr
    */
    tasks::HLSLCompilationTask* find(Key const& key);    

    /*! attempts to locate HLSL compilation task with provided key within the cache.
    If the search was not successful returns nullptr
    */
    tasks::HLSLCompilationTask const* find(Key const& key) const;

private:
    cache_storage m_tasks;
    cache_mapping m_tasks_cache_keys;
};


}}}}}

#define LEXGINE_CORE_DX_D3D12_TASK_CACHES_HLSL_COMPILATION_TASK_CACHE_H
#endif