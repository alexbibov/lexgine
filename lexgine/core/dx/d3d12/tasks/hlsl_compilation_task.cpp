#include "hlsl_compilation_task.h"
#include "../../../data_blob.h"
#include <d3dcompiler.h>

using namespace lexgine::core::dx::d3d12::tasks;

HLSLCompilationTask::HLSLCompilationTask(std::string const& source, std::string const& source_name,
    ShaderType shader_type, std::string const& shader_entry_point, lexgine::core::ShaderSourceCodePreprocessor::SourceType source_type,
    std::list<HLSLMacroDefinition> const& macro_definitions) :
    m_source{ source },
    m_source_name{ source_name },
    m_type{ shader_type },
    m_shader_entry_point{ shader_entry_point },
    m_source_type{ source_type },
    m_preprocessor_macro_definitions{ macro_definitions }
{
}

lexgine::core::D3DDataBlob HLSLCompilationTask::getTaskData() const
{
    return m_shader_bytecode;
}

bool lexgine::core::dx::d3d12::tasks::HLSLCompilationTask::do_task(uint8_t worker_id, uint16_t frame_index)
{
    std::string processed_sader_source{ ShaderSourceCodePreprocessor{ m_source, m_source_type }.getPreprocessedSource() };

    D3D_SHADER_MACRO* macro_definitions = new D3D_SHADER_MACRO[m_preprocessor_macro_definitions.size()];
    uint32_t i;
    std::list<HLSLMacroDefinition>::iterator p;
    for (p = m_preprocessor_macro_definitions.begin(), i = 0; p != m_preprocessor_macro_definitions.end(); ++p, ++i)
    {
        macro_definitions[i].Name = p->name.c_str();
        macro_definitions[i].Definition = p->value.c_str();
    }

    char* target = nullptr;
    switch (m_type)
    {
    case ShaderType::vertex:
        target = "vs_5_0";
        break;
    case ShaderType::hull:
        target = "hs_5_0";
        break;
    case ShaderType::domain:
        target = "ds_5_0";
        break;
    case ShaderType::geometry:
        target = "gs_5_0";
        break;
    case ShaderType::pixel:
        target = "ps_5_0";
        break;
    case ShaderType::compute:
        target = "cs_5_0";
        break;
    }


    UINT compilation_flags = D3DCOMPILE_AVOID_FLOW_CONTROL | D3DCOMPILE_ENABLE_STRICTNESS
        | D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY | D3DCOMPILE_IEEE_STRICTNESS
        | D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_RESOURCES_MAY_ALIAS | D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#ifdef _DEBUG
    compilation_flags = compilation_flags | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compilation_flags = compilation_flags | D3DCOMPILE_SKIP_VALIDATION | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif 

    ID3DBlob* p_shader_bytecode_blob, *p_compilation_errors_blob;
    LEXGINE_ERROR_LOG(this, D3DCompile(processed_sader_source.c_str(), processed_sader_source.size(),
        m_source_name.c_str(), macro_definitions, NULL, m_shader_entry_point.c_str(), target,
        compilation_flags, NULL, &p_shader_bytecode_blob, &p_compilation_errors_blob), S_OK);

    // if the call of D3DCompile(...) has failed this object then was set to erroneous state
    if (p_compilation_errors_blob)
    {
        char const* p_compilation_error_message = static_cast<char const*>(p_compilation_errors_blob->GetBufferPointer());
        misc::Log::retrieve()->out(p_compilation_error_message);  
    }
    else
    {
        m_shader_bytecode = D3DDataBlob{ Microsoft::WRL::ComPtr<ID3DBlob>{p_shader_bytecode_blob} };
    }

    return true;
}

lexgine::core::concurrency::TaskType HLSLCompilationTask::get_task_type() const
{
    return concurrency::TaskType::cpu;
}


