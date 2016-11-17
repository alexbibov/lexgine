#ifndef LEXGINE_CORE_DX_D3D12_HLSL_COMPILATION_TASK_H

#include "entity.h"
#include "class_names.h"
#include "shader_source_code_preprocessor.h"

#include <list>

namespace lexgine {namespace core {namespace dx {namespace d3d12 {

//! Implements compilation of provided HLSL source code
class HLSLCompilationTask : public NamedEntity<D3D12HLSLCompilationTask>
{
public:
    //! Macro definition consumed by HLSL compiler
    struct HLSLMacroDefinition
    {
        std::string name;    //!< name of the macro to define
        std::string value;    //!< value of the macro to define
    };

    /*! Established a new shader compilation task. The type of source is determined from parameters source and source_type.
     when source_type = file then source is interpreted as path to a file containing HLSL code to compile. If source_type = string then
     source is interpreted as a string containing HLSL source code
    */
    HLSLCompilationTask(std::string const& source, std::string const& source_name, std::list<HLSLMacroDefinition> const& macro_definitions,
        std::string const& shader_entry_point, ShaderSourceCodePreprocessor::SourceType source_type);



private:

};

}}}}

#define LEXGINE_CORE_DX_D3D12_HLSL_COMPILATION_TASK_H
#endif
