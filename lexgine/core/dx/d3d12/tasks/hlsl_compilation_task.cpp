#include "hlsl_compilation_task.h"
#include "../../../data_blob.h"
#include <d3dcompiler.h>
#include "../../../global_settings.h"
#include "../../../misc/hashed_string.h"


using namespace lexgine::core::dx::d3d12::tasks;
using namespace lexgine::core;

HLSLCompilationTask::HLSLCompilationTask(GlobalSettings const& global_settings, std::string const& source, std::string const& source_name,
    ShaderModel shader_model, ShaderType shader_type, std::string const& shader_entry_point, 
    lexgine::core::ShaderSourceCodePreprocessor::SourceType source_type,
    void* p_target_pso_descriptors, uint32_t num_descriptors,
    std::list<HLSLMacroDefinition> const& macro_definitions) :
    m_global_settings{ global_settings },
    m_source{ source },
    m_source_name{ source_name },
    m_shader_model{ shader_model },
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

bool HLSLCompilationTask::execute(uint8_t worker_id)
{
    return do_task(worker_id, 0);
}

bool lexgine::core::dx::d3d12::tasks::HLSLCompilationTask::do_task(uint8_t worker_id, uint16_t frame_index)
{
    // find the full correct path to the shader
    std::string full_shader_path{ m_source };
    for(auto path_prefix : m_global_settings.getShaderLookupDirectories())
        if (misc::doesFileExist(path_prefix + m_source))
        {
            full_shader_path = path_prefix + m_source;
            break;
        }

    ShaderSourceCodePreprocessor shader_preprocessor{ full_shader_path, m_source_type };

    if (shader_preprocessor.getErrorState())
    {
        raiseError("Unable to process shader source");
        return true;
    }

    std::string processed_shader_source{ shader_preprocessor.getPreprocessedSource() };

    D3D_SHADER_MACRO* macro_definitions = m_preprocessor_macro_definitions.size() ?
        new D3D_SHADER_MACRO[m_preprocessor_macro_definitions.size()] : nullptr;
    uint32_t i;
    std::list<HLSLMacroDefinition>::iterator p;
    for (p = m_preprocessor_macro_definitions.begin(), i = 0; p != m_preprocessor_macro_definitions.end(); ++p, ++i)
    {
        macro_definitions[i].Name = p->name.c_str();
        macro_definitions[i].Definition = p->value.c_str();
    }

    char* shader_type_prefix = nullptr;
    switch (m_type)
    {
    case ShaderType::vertex:
        shader_type_prefix = "vs_";
        break;
    case ShaderType::hull:
        shader_type_prefix = "hs_";
        break;
    case ShaderType::domain:
        shader_type_prefix = "ds_";
        break;
    case ShaderType::geometry:
        shader_type_prefix = "gs_";
        break;
    case ShaderType::pixel:
        shader_type_prefix = "ps_";
        break;
    case ShaderType::compute:
        shader_type_prefix = "cs_";
        break;
    }
    std::string target = shader_type_prefix 
        + std::to_string(static_cast<unsigned>(m_shader_model) / 10U) + "_"
        + std::to_string(static_cast<unsigned>(m_shader_model) % 10U);


    UINT compilation_flags = 
        D3DCOMPILE_AVOID_FLOW_CONTROL |
        D3DCOMPILE_IEEE_STRICTNESS | 
        D3DCOMPILE_WARNINGS_ARE_ERRORS | 
        (target[0] == 'c' ? D3DCOMPILE_RESOURCES_MAY_ALIAS : 0) | 
        D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#ifdef _DEBUG
    compilation_flags = compilation_flags | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compilation_flags = compilation_flags | D3DCOMPILE_SKIP_VALIDATION | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif 

    ID3DBlob* p_shader_bytecode_blob{ nullptr }, *p_compilation_errors_blob{ nullptr };
    
    std::string shader_source_for_hashing{ processed_shader_source + "_with_preprocessor_definitions_" };
    for (D3D_SHADER_MACRO* p_def = macro_definitions; 
        p_def < macro_definitions + m_preprocessor_macro_definitions.size();
        ++p_def)
    {
        shader_source_for_hashing += std::string{ "_name_" } +p_def->Name + "_value_" + p_def->Definition;
    }
    misc::HashedString shader_hashed_source{ shader_source_for_hashing.c_str() };

    std::string cached_shader_blob_name{ m_source + "_" + target + "_" 
        + std::to_string(shader_hashed_source.hash()) + ".shader" };
    std::string cached_blob_path{ m_global_settings.getCacheDirectory() + cached_shader_blob_name };

    if (misc::doesFileExist(cached_blob_path))
    {
        size_t compiled_shader_size = misc::getFileSize(cached_blob_path);
        D3DCreateBlob(compiled_shader_size, &p_shader_bytecode_blob);
        misc::readBinaryDataFromSourceFile(cached_blob_path, p_shader_bytecode_blob->GetBufferPointer());
    }
    else
    {
        LEXGINE_LOG_ERROR_IF_FAILED(this, D3DCompile(processed_shader_source.c_str(), processed_shader_source.size(),
            m_source_name.c_str(), macro_definitions, NULL, m_shader_entry_point.c_str(), target.c_str(),
            compilation_flags, NULL, &p_shader_bytecode_blob, &p_compilation_errors_blob), S_OK);
    }
    if (macro_definitions) delete[] macro_definitions;

    // if the call of D3DCompile(...) has failed this object then was set to erroneous state
    if (p_compilation_errors_blob)
    {
        m_compilation_log = std::string{ static_cast<char const*>(p_compilation_errors_blob->GetBufferPointer()) };
        std::string output_log = "Unable to compile shader source located in \"" + full_shader_path + "\". "
            "Detailed compiler log follows: <em>" + m_compilation_log + "</em>";
        LEXGINE_LOG_ERROR(this, output_log);
    }
    else
    {
        m_was_compilation_successful = true;

        misc::writeBinaryDataToFile(cached_blob_path, p_shader_bytecode_blob->GetBufferPointer(), p_shader_bytecode_blob->GetBufferSize());

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


