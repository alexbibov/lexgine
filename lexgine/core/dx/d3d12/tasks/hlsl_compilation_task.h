#ifndef LEXGINE_CORE_DX_D3D12_TASKS_HLSL_COMPILATION_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_HLSL_COMPILATION_TASK_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/shader_source_code_preprocessor.h"
#include "lexgine/core/data_blob.h"

#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"

#include "lexgine/core/dx/dxcompilation/common.h"
#include "lexgine/core/dx/dxcompilation/dx_compiler_proxy.h"
#include "lexgine/core/streamed_cache.h"

#include <list>

namespace lexgine {namespace core {namespace dx {namespace d3d12 {namespace tasks {

//! Implements compilation of provided HLSL source code
class HLSLCompilationTask : public concurrency::SchedulableTask
{
public:
    

    /*! Establishes a new shader compilation task. The type of source is determined from parameters source and source_type.
     when source_type = file then source is interpreted as path to a file containing HLSL code to compile. If source_type = string then
     source is interpreted as a string containing HLSL source code.
     
     Here p_target_pso_descriptors should point to a contiguous array of either graphics or compute descriptors that will receive 
     compiled shader blob. The exact type of PSO descriptor as well as its shader stage receiving the blob is determined based on
     provided value of parameter shader_type.
    */
    HLSLCompilationTask(core::Globals const& globals, std::string const& source, std::string const& source_name,
        dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
        ShaderSourceCodePreprocessor::SourceType source_type,
        void* p_target_pso_descriptors, uint32_t num_descriptors,
        std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions = std::list<dxcompilation::HLSLMacroDefinition>{},
        dxcompilation::HLSLCompilationOptimizationLevel optimization_level = dxcompilation::HLSLCompilationOptimizationLevel::level3,
        bool strict_mode = true, bool force_all_resources_be_bound = false,
        bool force_ieee_standard = true, bool treat_warnings_as_errors = true, bool enable_validation = true,
        bool enable_debug_information = false, bool enable_16bit_types = false);

    /*! Returns 'true' if the compilation was successful, returns 'false' if compilation has not been done yet or if it has failed (and the
     associated exception has been "eaten"). During execution of the task the return value is undefined.
    */
    bool wasSuccessful() const;

    /*! Returns compilation log string. If the compilation was successful or the task has not yet been executed returns empty string.
     During execution of the task the return value is undefined.
    */
    std::string getCompilationLog() const;


    // Executed the task manually. THROWS if compilation fails.
    bool execute(uint8_t worker_id);

private:
    struct ShaderCacheKey final
    {
        char source_path[2048U];
        uint16_t shader_model;
        uint64_t hash_value;
        

        static size_t const serialized_size = 
            2048U 
            + sizeof(uint16_t)
            + sizeof(uint64_t);


        std::string toString() const;

        void serialize(void* p_serialization_blob) const;
        void deserialize(void const* p_serialization_blob);

        ShaderCacheKey(std::string const& hlsl_source_path, 
            uint16_t shader_model,
            uint64_t hash_value);
        ShaderCacheKey() = default;

        bool operator<(ShaderCacheKey const& other) const;
        bool operator==(ShaderCacheKey const& other) const;
    };

private:
    bool do_task(uint8_t worker_id, uint16_t frame_index) override;    //! performs actual compilation of the shader
    concurrency::TaskType get_task_type() const override;    //! returns type of this task (CPU)

    GlobalSettings const& m_global_settings;
    dxcompilation::DXCompilerProxy& m_dxc_proxy;


    std::string m_source;
    std::string m_source_name;
    dxcompilation::ShaderModel m_shader_model;
    dxcompilation::ShaderType m_type;
    std::string m_shader_entry_point;
    ShaderSourceCodePreprocessor::SourceType m_source_type;
    void* m_p_target_pso_descriptors;
    uint32_t m_num_target_descriptors;
    
    std::list<dxcompilation::HLSLMacroDefinition> m_preprocessor_macro_definitions;
    dxcompilation::HLSLCompilationOptimizationLevel m_optimization_level;
    bool m_is_strict_mode_enabled;
    bool m_is_all_resources_binding_forced;
    bool m_is_ieee_forced;
    bool m_are_warnings_treated_as_errors;
    bool m_is_validation_enabled;
    bool m_should_enable_debug_information;
    bool m_should_enable_16bit_types;

    bool m_was_compilation_successful;
    std::string m_compilation_log;
};

}}}}}


#endif
