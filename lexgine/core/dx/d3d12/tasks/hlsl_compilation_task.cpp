#include "hlsl_compilation_task.h"
#include "../../../data_blob.h"
#include <d3dcompiler.h>

using namespace lexgine::core::dx::d3d12::tasks;

HLSLCompilationTask::HLSLCompilationTask(std::string const& source, std::string const& source_name,
    ShaderType shader_type, std::string const& shader_entry_point, lexgine::core::ShaderSourceCodePreprocessor::SourceType source_type,
    void* p_target_pso_descriptors, uint32_t num_descriptors,
    std::list<HLSLMacroDefinition> const& macro_definitions) :
    m_source{ source },
    m_source_name{ source_name },
    m_type{ shader_type },
    m_shader_entry_point{ shader_entry_point },
    m_source_type{ source_type },
    m_p_target_pso_descriptors{ p_target_pso_descriptors },
    m_num_target_descriptors{ num_descriptors },
    m_preprocessor_macro_definitions{ macro_definitions },
    m_was_compilation_successful{ false },
    m_compilation_log{ "" }
{
    Entity::setStringName(source_name);
}


bool HLSLCompilationTask::wasSuccessful() const
{
    return m_was_compilation_successful;
}

std::string HLSLCompilationTask::getCompilationLog() const
{
    return m_compilation_log;
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
        m_compilation_log = std::string{ static_cast<char const*>(p_compilation_errors_blob->GetBufferPointer()) };
        misc::Log::retrieve()->out(m_compilation_log);  
    }
    else
    {
        m_was_compilation_successful = true;

        D3DDataBlob shader_bytecode{ Microsoft::WRL::ComPtr<ID3DBlob>{p_shader_bytecode_blob} };

        if (m_type == ShaderType::compute)
        {
            ComputePSODescriptor* p_compute_pso_descriptors = static_cast<ComputePSODescriptor*>(m_p_target_pso_descriptors);
            
            for (uint32_t i = 0; i < m_num_target_descriptors; ++i)
                p_compute_pso_descriptors[i].compute_shader = shader_bytecode;
        }
        else
        {
            GraphicsPSODescriptor* p_graphics_pso_descriptors = static_cast<GraphicsPSODescriptor*>(m_p_target_pso_descriptors);

            for(uint32_t i = 0; i < m_num_target_descriptors; ++i)
            {
                switch (m_type)
                {
                case ShaderType::vertex:
                    p_graphics_pso_descriptors[i].vertex_shader = shader_bytecode;
                    break;
                case ShaderType::hull:
                    p_graphics_pso_descriptors[i].hull_shader = shader_bytecode;
                    break;
                case ShaderType::domain:
                    p_graphics_pso_descriptors[i].domain_shader = shader_bytecode;
                    break;
                case ShaderType::geometry:
                    p_graphics_pso_descriptors[i].geometry_shader = shader_bytecode;
                    break;
                case ShaderType::pixel:
                    p_graphics_pso_descriptors[i].pixel_shader = shader_bytecode;
                    break;
                }
            }
        }
    }

    return true;
}

lexgine::core::concurrency::TaskType HLSLCompilationTask::get_task_type() const
{
    return concurrency::TaskType::cpu;
}


