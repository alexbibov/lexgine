#ifndef LEXGINE_CORE_DX_D3D12_TASKS_HLSL_COMPILATION_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_HLSL_COMPILATION_TASK_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/data_blob.h"
#include "lexgine/core/streamed_cache.h"

#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/misc/datetime.h"

#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/task_caches/combined_cache_key.h"
#include "lexgine/core/dx/dxcompilation/common.h"
#include "lexgine/core/dx/dxcompilation/dx_compiler_proxy.h"


#include <list>

namespace lexgine::core::dx::d3d12::tasks {

//! Implements compilation of provided HLSL source code
class HLSLCompilationTask : public concurrency::SchedulableTask
{
public:
    /*! Deploys new shader compilation task. The type of source is determined from parameters source and source_type.
     when source_type = file then source is interpreted as path to a file containing HLSL code to compile. If source_type = string then
     source is interpreted as a string containing HLSL source code.
     
     Here p_target_pso_descriptors should point to a contiguous array of either graphics or compute descriptors that will receive 
     compiled shader blob. The exact type of PSO descriptor as well as its shader stage receiving the blob is determined based on
     provided value of parameter shader_type.
    */
    HLSLCompilationTask(task_caches::CombinedCacheKey const& key, misc::DateTime const& time_stamp,
        core::Globals& globals, std::string const& hlsl_source, std::string const& source_name,
        dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
        std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions = std::list<dxcompilation::HLSLMacroDefinition>{},
        dxcompilation::HLSLCompilationOptimizationLevel optimization_level = dxcompilation::HLSLCompilationOptimizationLevel::level3,
        bool strict_mode = true, bool force_all_resources_be_bound = false,
        bool force_ieee_standard = true, bool treat_warnings_as_errors = true, bool enable_validation = true,
        bool enable_debug_information = false, bool enable_16bit_types = false);

    //! Returns 'true' if the task has completed successfully
    bool wasSuccessful() const;

    //! Returns 'true' if shader DXIL code was loaded from cache and not actually compiled
    bool isPrecached() const;

    /*! Returns compilation log string. If the compilation was successful or the task has not yet been executed returns empty string.
     During execution of the task the return value is undefined.
    */
    std::string getCompilationLog() const;


    //! Executes the task manually and returns 'true' on success
    bool execute(uint8_t worker_id);


    //! Returns blob containing compiled shader byte code
    D3DDataBlob getTaskData() const;


    /*!
      Returns string name associated with HLSL compilation task in the shader cache.
      The name follows convention provided below:
            {<source_shader_path>}__{<hash_value>}__{<shader_type_and_shading_model>}
      Note that naming for shader compilation task is different from PSO and root signature
      compilation tasks and is not completely "predictable" since the hash value is generated
      automatically when the compilation task is inserted into the cache. Therefore, this
      function may only be used for reference or debugging purposes and the user should never
      make any assumptions regarding how the final cache name for the shader compilation task will look.
    */
    std::string getCacheName() const;

private:
public:
    static std::pair<uint8_t, uint8_t> unpackShaderModelVersion(dxcompilation::ShaderModel shader_model);
    static std::string shaderModelAndTypeToTargetName(dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type);

private:
    bool do_task(uint8_t worker_id, uint16_t frame_index) override;    //! performs actual compilation of the shader
    concurrency::TaskType get_task_type() const override;    //! returns type of this task (CPU)

    task_caches::CombinedCacheKey const& m_key;
    misc::DateTime const m_time_stamp;
    GlobalSettings const& m_global_settings;
    dxcompilation::DXCompilerProxy& m_dxc_proxy;

    std::string m_hlsl_source;
    std::string m_source_name;
    dxcompilation::ShaderModel m_shader_model;
    dxcompilation::ShaderType m_shader_type;
    std::string m_shader_entry_point;
    
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

    bool mutable m_should_recompile;
    D3DDataBlob m_shader_byte_code;
};

}


#endif
