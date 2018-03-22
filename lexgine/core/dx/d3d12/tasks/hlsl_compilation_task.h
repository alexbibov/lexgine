#ifndef LEXGINE_CORE_DX_D3D12_TASKS_HLSL_COMPILATION_TASK_H

#include "../../../concurrency/schedulable_task.h"
#include "../../../shader_source_code_preprocessor.h"
#include "../../../data_blob.h"
#include "../pipeline_state.h"
#include "../../../lexgine_core_fwd.h"

#include <list>

namespace lexgine {namespace core {namespace dx {namespace d3d12 {namespace tasks {

enum class ShaderType
{
    vertex = 0, hull = 1, domain = 2, geometry = 3, pixel = 4, compute = 5,
    tessellation_control = 1, tesselation_evaluation = 2, fragment = 4    // OpenGL/Vulkan terminology for convenience
};

enum class ShaderModel : unsigned
{
    model_50 = 5,
    model_60 = 6,
    model_61 = (1 << 4) + 6,
    model_62 = (2 << 4) + 6
};

enum class HLSLCompilationWarningLevel : unsigned char
{
    level_no = static_cast<unsigned char>(-1),
    level0 = 0,
    level1,
    level2,
    level3,
    level4
};

//! Implements compilation of provided HLSL source code
class HLSLCompilationTask : public concurrency::SchedulableTask
{
public:
    //! Macro definition consumed by HLSL compiler
    struct HLSLMacroDefinition
    {
        std::string name;    //!< name of the macro to define
        std::string value;    //!< value of the macro to define
    };

    /*! Establishes a new shader compilation task. The type of source is determined from parameters source and source_type.
     when source_type = file then source is interpreted as path to a file containing HLSL code to compile. If source_type = string then
     source is interpreted as a string containing HLSL source code.
     
     Here p_target_pso_descriptors should point to a contiguous array of either graphics or compute descriptors that will receive 
     compiled shader blob. The exact type of PSO descriptor as well as its shader stage receiving the blob is determined based on
     provided value of parameter shader_type.
    */
    HLSLCompilationTask(GlobalSettings const& global_settings, std::string const& source, std::string const& source_name,
        ShaderModel shader_model, ShaderType shader_type, std::string const& shader_entry_point, ShaderSourceCodePreprocessor::SourceType source_type,
        void* p_target_pso_descriptors, uint32_t num_descriptors,
        std::list<HLSLMacroDefinition> const& macro_definitions = std::list<HLSLMacroDefinition>{});

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
    bool do_task(uint8_t worker_id, uint16_t frame_index) override;    //! performs actual compilation of the shader
    concurrency::TaskType get_task_type() const override;    //! returns type of this task (CPU)

    GlobalSettings const& m_global_settings;
    std::string m_source;
    std::string m_source_name;
    ShaderModel m_shader_model;
    ShaderType m_type;
    std::string m_shader_entry_point;
    ShaderSourceCodePreprocessor::SourceType m_source_type;
    void* m_p_target_pso_descriptors;
    uint32_t m_num_target_descriptors;
    std::list<HLSLMacroDefinition> m_preprocessor_macro_definitions;

    bool m_was_compilation_successful;
    std::string m_compilation_log;
};

}}}}}

#define LEXGINE_CORE_DX_D3D12_TASKS_HLSL_COMPILATION_TASK_H
#endif
