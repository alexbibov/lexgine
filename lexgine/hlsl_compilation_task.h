#ifndef LEXGINE_CORE_DX_D3D12_TASKS_HLSL_COMPILATION_TASK_H

#include "schedulable_task.h"
#include "shader_source_code_preprocessor.h"
#include "data_blob.h"

#include <list>

namespace lexgine {namespace core {namespace dx {namespace d3d12 {namespace tasks {

enum class ShaderType
{
    vertex = 0, hull = 1, domain = 2, geometry = 3, pixel = 4, compute = 5,
    tessellation_control = 1, tesselation_evaluation = 2, fragment = 4    // OpenGL/Vulkan terminology for convenience
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
     source is interpreted as a string containing HLSL source code
    */
    HLSLCompilationTask(std::string const& source, std::string const& source_name,
        ShaderType shader_type, std::string const& shader_entry_point, ShaderSourceCodePreprocessor::SourceType source_type,
        std::list<HLSLMacroDefinition> const& macro_definitions = std::list<HLSLMacroDefinition>{});



private:
    bool do_task(uint8_t worker_id, uint16_t frame_index) override;    //! performs actual compilation of the shader
    concurrency::TaskType get_task_type() const override;    //! returns type of this task (CPU)

    std::string m_source;
    std::string m_source_name;
    ShaderType m_type;
    std::string m_shader_entry_point;
    ShaderSourceCodePreprocessor::SourceType m_source_type;
    std::list<HLSLMacroDefinition> m_preprocessor_macro_definitions;

    D3DDataBlob m_shader_bytecode;
};

}}}}}

#define LEXGINE_CORE_DX_D3D12_TASKS_HLSL_COMPILATION_TASK_H
#endif
