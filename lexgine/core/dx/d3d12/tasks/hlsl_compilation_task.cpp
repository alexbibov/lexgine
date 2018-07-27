#include "hlsl_compilation_task.h"
#include "lexgine/core/data_blob.h"

#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/misc/hashed_string.h"
#include "lexgine/core/exception.h"

#include "lexgine/core/dx/d3d12/dx_resource_factory.h"
#include "lexgine/core/dx/d3d12/pipeline_state.h"
#include "lexgine/core/dx/d3d12/task_caches/cache_utilities.h"

#include <d3dcompiler.h>
#include <fstream>


using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx::dxcompilation;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks;


std::string HLSLCompilationTask::getCacheName() const
{
    return m_key.toString();
}

std::pair<uint8_t, uint8_t> lexgine::core::dx::d3d12::tasks::HLSLCompilationTask::unpackShaderModelVersion(ShaderModel shader_model)
{
    uint8_t version_major = static_cast<uint8_t>((static_cast<unsigned short>(shader_model) >> 4) & 0xF);
    uint8_t version_minor = static_cast<uint8_t>(static_cast<unsigned short>(shader_model) & 0xF);

    return std::make_pair(version_major, version_minor);
}

std::string lexgine::core::dx::d3d12::tasks::HLSLCompilationTask::shaderModelAndTypeToTargetName(ShaderModel shader_model, ShaderType shader_type)
{
    char target[7] = { 0 };

    std::pair<uint8_t, uint8_t> shader_model_version = unpackShaderModelVersion(shader_model);

    switch (shader_type)
    {
    case ShaderType::vertex:
        memcpy(target, "vs_", 3);
        break;
    case ShaderType::hull:
        memcpy(target, "hs_", 3);
        break;
    case ShaderType::domain:
        memcpy(target, "ds_", 3);
        break;
    case ShaderType::geometry:
        memcpy(target, "gs_", 3);
        break;
    case ShaderType::pixel:
        memcpy(target, "ps_", 3);
        break;
    case ShaderType::compute:
        memcpy(target, "cs_", 3);
        break;
    }

    target[3] = '0' + shader_model_version.first;
    target[4] = '_';
    target[5] = '0' + shader_model_version.second;

    return target;
}



HLSLCompilationTask::HLSLCompilationTask(task_caches::CombinedCacheKey const& key, misc::DateTime const& time_stamp,
    core::Globals& globals, std::string const& hlsl_source, std::string const& source_name,
    ShaderModel shader_model, ShaderType shader_type, std::string const& shader_entry_point,
    std::list<HLSLMacroDefinition> const& macro_definitions/* = std::list<HLSLMacroDefinition>{}*/,
    HLSLCompilationOptimizationLevel optimization_level/* = HLSLCompilationOptimizationLevel::level3*/,
    bool strict_mode/* = true*/, bool force_all_resources_be_bound/* = false*/,
    bool force_ieee_standard/* = true*/, bool treat_warnings_as_errors/* = true*/, bool enable_validation/* = true*/,
    bool enable_debug_information/* = false*/, bool enable_16bit_types/* = false*/) :
    m_key{ key },
    m_time_stamp{ time_stamp },
    m_global_settings{ *globals.get<GlobalSettings>() },
    m_dxc_proxy{ globals.get<DxResourceFactory>()->RetrieveSM6DxCompilerProxy() },
    m_hlsl_source{ hlsl_source },
    m_source_name{ source_name },
    m_shader_model{ shader_model },
    m_shader_type{ shader_type },
    m_shader_entry_point{ shader_entry_point },
    m_preprocessor_macro_definitions{ macro_definitions },
    m_optimization_level{ optimization_level },
    m_is_strict_mode_enabled{ strict_mode },
    m_is_all_resources_binding_forced{ force_all_resources_be_bound },
    m_is_ieee_forced{ force_ieee_standard },
    m_are_warnings_treated_as_errors{ treat_warnings_as_errors },
    m_is_validation_enabled{ enable_validation },
    m_should_enable_debug_information{ enable_debug_information },
    m_should_enable_16bit_types{ enable_16bit_types },
    m_was_compilation_successful{ false },
    m_compilation_log{ "" },
    m_should_recompile{ true }
{
    Entity::setStringName(source_name);

    std::pair<uint8_t, uint8_t> shader_model_version = unpackShaderModelVersion(m_shader_model);

    if (shader_model_version.first < 6)
    {
        if (optimization_level == HLSLCompilationOptimizationLevel::level4)
        {
            misc::Log::retrieve()->out("Legacy HLSL compiler does not support optimization level 4. "
                "Optimization will be forced to level 3", misc::LogMessageType::exclamation);
            m_optimization_level = HLSLCompilationOptimizationLevel::level3;
        }

        if (enable_16bit_types)
        {
            misc::Log::retrieve()->out("Legacy HLSL compiler does not support 16-bit reduced precision types. "
                "This capability will be forced to \"false\"", misc::LogMessageType::exclamation);
            m_should_enable_16bit_types = false;
        }
    }
    else if (shader_model_version.first == 6 && shader_model_version.second < 2)
    {
        if (enable_16bit_types)
        {
            misc::Log::retrieve()->out("Reduced precision 16-bit types require shader model 6.2 although shader model "
                + std::to_string(shader_model_version.first) + "." + std::to_string(shader_model_version.second)
                + " was requested. The shader model will be forced to 6.2", misc::LogMessageType::exclamation);
            m_shader_model = ShaderModel::model_62;
        }
    }

    if (m_should_enable_debug_information)
    {
        m_optimization_level = HLSLCompilationOptimizationLevel::level_no;
        m_is_validation_enabled = true;
    }
}


bool HLSLCompilationTask::wasSuccessful() const
{
    return m_was_compilation_successful || !m_should_recompile;
}

bool HLSLCompilationTask::isPrecached() const
{
    return !m_should_recompile;
}

std::string HLSLCompilationTask::getCompilationLog() const
{
    return m_compilation_log;
}

bool HLSLCompilationTask::execute(uint8_t worker_id)
{
    return do_task(worker_id, 0);
}

D3DDataBlob HLSLCompilationTask::getTaskData() const
{
    return m_shader_byte_code;
}

bool HLSLCompilationTask::do_task(uint8_t worker_id, uint16_t frame_index)
{
    try
    {
        auto shader_cache_containing_requested_shader =
            task_caches::findCombinedCacheContainingKey(m_key, m_global_settings);

        if (shader_cache_containing_requested_shader.isValid())
        {
            misc::DateTime cached_time_stamp =
                static_cast<task_caches::StreamedCacheConnection&>(shader_cache_containing_requested_shader).cache().getEntryTimestamp(m_key);

            m_should_recompile = cached_time_stamp < m_time_stamp;
        }
        else m_should_recompile = true;

        if (!m_should_recompile)
        {
            // Attempt to use cached version of the shader

            SharedDataChunk blob =
                static_cast<task_caches::StreamedCacheConnection&>(shader_cache_containing_requested_shader).cache().retrieveEntry(m_key);
            if (!blob.data())
            {
                LEXGINE_LOG_ERROR(this, "Unable to retrieve precompiled shader byte code for source \""
                    + m_source_name + "\"");
                m_should_recompile = true;
            }
            else
            {
                Microsoft::WRL::ComPtr<ID3DBlob> d3d_blob{ nullptr };
                HRESULT hres = D3DCreateBlob(blob.size(), d3d_blob.GetAddressOf());
                if (hres != S_OK && hres != S_FALSE)
                {
                    LEXGINE_LOG_ERROR(this, "Unable to create D3D blob to store precompiled shader code for source \""
                        + m_source_name + "\"");
                    m_should_recompile = true;
                }
                else
                {
                    memcpy(d3d_blob->GetBufferPointer(), blob.data(), blob.size());
                    m_shader_byte_code = D3DDataBlob{ d3d_blob };
                }
            }
        }


        if (m_should_recompile)
        {
            // Need to recompile the shader
            std::string target = shaderModelAndTypeToTargetName(m_shader_model, m_shader_type);

            if (static_cast<unsigned short>(m_shader_model) < static_cast<unsigned short>(ShaderModel::model_60))
            {
                // use legacy compiler

                // blobs that will receive compilation errors and DXIL byte code
                Microsoft::WRL::ComPtr<ID3DBlob> p_shader_bytecode_blob{ nullptr };
                Microsoft::WRL::ComPtr<ID3DBlob> p_compilation_errors_blob{ nullptr };

                std::vector<D3D_SHADER_MACRO> macro_definitions{};
                macro_definitions.resize(m_preprocessor_macro_definitions.size() + 1);
                macro_definitions[m_preprocessor_macro_definitions.size()] = D3D_SHADER_MACRO{ NULL, NULL };
                {
                    uint32_t i;
                    std::list<HLSLMacroDefinition>::iterator p;
                    for (p = m_preprocessor_macro_definitions.begin(), i = 0; p != m_preprocessor_macro_definitions.end(); ++p, ++i)
                    {
                        macro_definitions[i].Name = p->name.c_str();
                        macro_definitions[i].Definition = p->value.c_str();
                    }
                }


                UINT compilation_flags =
                    D3DCOMPILE_AVOID_FLOW_CONTROL
                    | (target[0] == 'c' ? D3DCOMPILE_RESOURCES_MAY_ALIAS : 0U)    // UAV and SRV resources may alias for cs_5_0
                    | D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES
                    | (m_is_strict_mode_enabled ? D3DCOMPILE_ENABLE_STRICTNESS : 0U)
                    | (m_is_all_resources_binding_forced ? D3DCOMPILE_ALL_RESOURCES_BOUND : 0U)
                    | (m_is_ieee_forced ? D3DCOMPILE_IEEE_STRICTNESS : 0U)
                    | (m_are_warnings_treated_as_errors ? D3DCOMPILE_WARNINGS_ARE_ERRORS : 0U)
                    | (m_is_validation_enabled ? 0U : D3DCOMPILE_SKIP_VALIDATION)
                    | (m_should_enable_debug_information ? D3DCOMPILE_DEBUG : 0U);


                switch (m_optimization_level)
                {
                case HLSLCompilationOptimizationLevel::level_no:
                    compilation_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
                    break;
                case HLSLCompilationOptimizationLevel::level0:
                    compilation_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
                    break;
                case HLSLCompilationOptimizationLevel::level1:
                    compilation_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
                    break;
                case HLSLCompilationOptimizationLevel::level2:
                    compilation_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL2;
                    break;
                case HLSLCompilationOptimizationLevel::level3:
                    compilation_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
                    break;
                }


                LEXGINE_LOG_ERROR_IF_FAILED(
                    this,
                    D3DCompile(m_hlsl_source.c_str(), m_hlsl_source.size(),
                        m_source_name.c_str(), macro_definitions.data(), NULL, m_shader_entry_point.c_str(), target.c_str(),
                        compilation_flags, NULL, p_shader_bytecode_blob.GetAddressOf(), p_compilation_errors_blob.GetAddressOf()),
                    S_OK);

                if (p_compilation_errors_blob)
                {
                    m_compilation_log = std::string{ static_cast<char const*>(p_compilation_errors_blob->GetBufferPointer()) };
                    std::string output_log = "Unable to compile shader source \"" + m_source_name + "\". "
                        "Detailed compiler log follows: <em>" + m_compilation_log + "</em>";
                    LEXGINE_LOG_ERROR(this, output_log);
                    m_was_compilation_successful = false;
                }
                else
                {
                    m_shader_byte_code = D3DDataBlob{ p_shader_bytecode_blob };
                    m_was_compilation_successful = true;
                }

            }
            else
            {
                // use LLVM-based compiler with SM6 support

                if (!m_dxc_proxy.compile(worker_id, m_hlsl_source, m_source_name, m_shader_entry_point, target,
                    m_preprocessor_macro_definitions, m_optimization_level, m_is_strict_mode_enabled,
                    m_is_all_resources_binding_forced, m_is_ieee_forced, m_are_warnings_treated_as_errors,
                    m_is_validation_enabled, m_should_enable_debug_information, m_should_enable_16bit_types))
                {
                    LEXGINE_LOG_ERROR(this, "Compilation of HLSL source \"" + m_source_name
                        + "\" has failed (details: <em>" + m_dxc_proxy.errors(worker_id) + "</em>)");
                    m_was_compilation_successful = false;
                }
                else
                {
                    auto compilation_result = m_dxc_proxy.result(worker_id);
                    if (compilation_result.isValid())
                    {
                        m_shader_byte_code = static_cast<D3DDataBlob>(compilation_result);
                        m_was_compilation_successful = true;
                    }
                    else
                    {
                        LEXGINE_LOG_ERROR(this, "Unable to retrieve DXIL byte code for shader source \"" + m_source_name + "\"");
                        m_was_compilation_successful = false;
                    }

                }

            }

            if (m_was_compilation_successful)
            {
                // if compilation was successful serialize compiled shader into the cache
                auto my_shader_cache =
                    task_caches::establishConnectionWithCombinedCache(m_global_settings, worker_id, false);

                my_shader_cache.cache().addEntry(task_caches::CombinedCache::entry_type{ m_key, m_shader_byte_code });
            }
        }
    }
    catch (Exception const& e)
    {
        LEXGINE_LOG_ERROR(this, std::string{ "Lexgine has thrown exception: " } +e.what());
    }
    catch (...)
    {
        LEXGINE_LOG_ERROR(this, "unknown exception");
    }

    return true;    // the task is not reschedulable, so do_task() returns 'true' regardless of compilation outcome
}

lexgine::core::concurrency::TaskType HLSLCompilationTask::get_task_type() const
{
    return concurrency::TaskType::cpu;
}


