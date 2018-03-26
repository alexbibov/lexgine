#include "hlsl_compilation_task.h"
#include "lexgine/core/data_blob.h"

#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/misc/hashed_string.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/misc/strict_weak_ordering.h"

#include "lexgine/core/dx/d3d12/dx_resource_factory.h"
#include "lexgine/core/dx/d3d12/pipeline_state.h"


#include <d3dcompiler.h>
#include <fstream>


using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx::dxcompilation;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks;

namespace {

std::pair<uint8_t, uint8_t> unpackShaderModelVersion(ShaderModel shader_model)
{
    uint8_t version_major = static_cast<uint8_t>((static_cast<unsigned short>(shader_model) >> 4) & 0xF);
    uint8_t version_minor = static_cast<uint8_t>(static_cast<unsigned short>(shader_model) & 0xF);

    return std::make_pair(version_major, version_minor);
}

std::string shaderModelAndTypeToTargetName(ShaderModel shader_model, ShaderType shader_type)
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



}


HLSLCompilationTask::HLSLCompilationTask(core::Globals const& globals, std::string const& source, std::string const& source_name,
    ShaderModel shader_model, ShaderType shader_type, std::string const& shader_entry_point,
    ShaderSourceCodePreprocessor::SourceType source_type,
    void* p_target_pso_descriptors, uint32_t num_descriptors,
    std::list<HLSLMacroDefinition> const& macro_definitions/* = std::list<HLSLMacroDefinition>{}*/,
    HLSLCompilationOptimizationLevel optimization_level/* = HLSLCompilationOptimizationLevel::level3*/,
    bool strict_mode/* = true*/, bool force_all_resources_be_bound/* = false*/,
    bool force_ieee_standard/* = true*/, bool treat_warnings_as_errors/* = true*/, bool enable_validation/* = true*/,
    bool enable_debug_information/* = false*/, bool enable_16bit_types/* = false*/) :
    m_global_settings{ *globals.get<GlobalSettings>() },
    m_dxc_proxy{ globals.get<DxResourceFactory>()->RetrieveSM6DxCompilerProxy() },
    m_source{ source },
    m_source_name{ source_name },
    m_shader_model{ shader_model },
    m_type{ shader_type },
    m_shader_entry_point{ shader_entry_point },
    m_source_type{ source_type },
    m_p_target_pso_descriptors{ p_target_pso_descriptors },
    m_num_target_descriptors{ num_descriptors },
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
    m_compilation_log{ "" }
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

std::pair<misc::HashedString, ShaderSourceCodePreprocessor::SourceType> HLSLCompilationTask::hash() const
{
    return std::make_pair(misc::HashedString{ m_source }, m_source_type);
}

std::string HLSLCompilationTask::ShaderCacheKey::toString() const
{
    return std::string{ std::string{"path:{"} +source_path + "}__hash:{" + std::to_string(hash_value) + "}" };
}

void HLSLCompilationTask::ShaderCacheKey::serialize(void* p_serialization_blob) const
{
    uint8_t* ptr{ static_cast<uint8_t*>(p_serialization_blob) };
    
    memcpy(ptr, source_path, sizeof(source_path)); ptr += sizeof(source_path);
    memcpy(ptr, &shader_model, sizeof(shader_model)); ptr += sizeof(shader_model);
    memcpy(ptr, &hash_value, sizeof(hash_value));
}

void HLSLCompilationTask::ShaderCacheKey::deserialize(void const* p_serialization_blob)
{
    uint8_t const* ptr{static_cast<uint8_t const*>(p_serialization_blob) };

    memcpy(source_path, ptr, sizeof(source_path)); ptr += sizeof(source_path);
    memcpy(&shader_model, ptr, sizeof(shader_model)); ptr += sizeof(shader_model);
    memcpy(&hash_value, ptr, sizeof(hash_value));
}

HLSLCompilationTask::ShaderCacheKey::ShaderCacheKey(std::string const& hlsl_source_path, 
    uint16_t shader_model,
    uint64_t hash_value) :
    shader_model{ shader_model },
    hash_value{ hash_value }
{
    memset(source_path, 0, sizeof(source_path));
    memcpy(source_path, hlsl_source_path.c_str(), hlsl_source_path.length());
}

bool HLSLCompilationTask::ShaderCacheKey::operator<(ShaderCacheKey const& other) const
{
    int r = std::strcmp(source_path, other.source_path);
    SWO_STEP(r, < , 0);
    SWO_STEP(shader_model, < , other.shader_model);
    SWO_END(hash_value, < , other.hash_value);
}

bool HLSLCompilationTask::ShaderCacheKey::operator==(ShaderCacheKey const& other) const
{
    return strcmp(source_path, other.source_path) == 0
        && shader_model == other.shader_model
        && hash_value == other.hash_value;
}

bool HLSLCompilationTask::do_task(uint8_t worker_id, uint16_t frame_index)
{
    // Initialize shader caches

    using shader_cache_type = StreamedCache<ShaderCacheKey, 256>;
    std::list<std::fstream> shader_cache_streams{};
    std::list<shader_cache_type> shader_caches{};

    std::string path_to_shader_cache_owned_by_current_thread = 
        m_global_settings.getCacheDirectory() + m_global_settings.getShaderCacheName() 
        + ".thread" + std::to_string(worker_id);
    bool cache_exists = misc::doesFileExist(path_to_shader_cache_owned_by_current_thread);
    
    {
        // open stream to the shader cache owned by current thread

        auto shader_cache_file_stream_flags = std::fstream::in | std::fstream::out | std::fstream::binary;
        if (!cache_exists) shader_cache_file_stream_flags |= std::fstream::trunc;
        shader_cache_streams.emplace_back(path_to_shader_cache_owned_by_current_thread, shader_cache_file_stream_flags);
        if (!shader_cache_streams.front())
        {
            LEXGINE_THROW_ERROR("Unable to initialize shader cache stream \""
                + path_to_shader_cache_owned_by_current_thread + "\"");
        }
    }

    if (cache_exists)
    {
        shader_cache_type cache{ shader_cache_streams.front(), false };
        if (!cache)
        {
            // if shader cache cannot be initialized, this is probably due to 
            // cache corruption. Therefore, we have to erase the old cache and create new one
            shader_cache_streams.front().close();
            shader_cache_streams.front() = std::fstream{
                path_to_shader_cache_owned_by_current_thread,
                std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc };
            shader_caches.emplace_back(
                shader_cache_streams.front(),
                m_global_settings.getMaxShaderCacheSize(),
                StreamedCacheCompressionLevel::level3,
                false);
        }
        else shader_caches.emplace_back(std::move(cache));
    }
    else
    {
        shader_caches.emplace_back(shader_cache_type{
            shader_cache_streams.front(),
            m_global_settings.getMaxShaderCacheSize(),
            StreamedCacheCompressionLevel::level3,
            false });
    }

    {
        // populate list of shader caches owned by other threads
        std::list<std::string> cache_library_names =
            misc::getFilesInDirectory(m_global_settings.getCacheDirectory(), "*.shaders.thread*");

        for (auto& lib_name : cache_library_names)
        {
            std::string lib_full_path = m_global_settings.getCacheDirectory() + lib_name;
            if (lib_full_path != path_to_shader_cache_owned_by_current_thread)
            {
                shader_cache_streams.emplace_back(lib_full_path.c_str(), std::fstream::in | std::fstream::binary);
                if (shader_cache_streams.back())
                {
                    shader_cache_type cache{ shader_cache_streams.back(), true };
                    if (cache) shader_caches.emplace_back(std::move(cache));
                    else shader_cache_streams.pop_back();
                }
                else
                {
                    shader_cache_streams.pop_back();
                }
            }
        }
    }



    // find the full correct path to the shader
    std::string full_shader_path{ m_source };
    for(auto path_prefix : m_global_settings.getShaderLookupDirectories())
    {
        if (misc::doesFileExist(path_prefix + m_source))
        {
            full_shader_path = path_prefix + m_source;
            break;
        }
    }
    ShaderSourceCodePreprocessor shader_preprocessor{ full_shader_path, m_source_type };
    std::string processed_shader_source{ shader_preprocessor.getPreprocessedSource() };
    std::string target = shaderModelAndTypeToTargetName(m_shader_model, m_type);

    ShaderCacheKey shader_cache_key{};
    {
        // Generate key for caching

        // note that theoretically two separate sets of defines may end up having same
        // hash value, which is not good, however probability of such occasion is essentially 0
        // and even if such thing happens it can be debugged and resolved easily (just by, say, changing name or value
        // of offending define)
        std::string shader_defines{};
        for (auto const& def : m_preprocessor_macro_definitions)
            shader_defines += std::string{ "DEFINE { name = " } +def.name + ", value = " + def.value + " }\n";
        uint64_t shader_hash_value = misc::HashedString{ shader_defines.c_str() }.hash();


        shader_cache_key = ShaderCacheKey{ full_shader_path, static_cast<uint16_t>(m_shader_model), shader_hash_value };
    }
    
    bool should_recompile{ false };

    auto p_shader_cache_containing_requested_shader = 
        std::find_if(shader_caches.begin(), shader_caches.end(),
            [&shader_cache_key](shader_cache_type& cache)
            {
                return cache.doesEntryExist(shader_cache_key);
            });

    if (p_shader_cache_containing_requested_shader != shader_caches.end())
    {
        misc::DateTime cached_time_stamp = p_shader_cache_containing_requested_shader->getEntryTimestamp(shader_cache_key);
        misc::Optional<misc::DateTime> current_time_stamp = misc::getFileLastUpdatedTimeStamp(full_shader_path);
        should_recompile = !current_time_stamp.isValid()
            || cached_time_stamp < static_cast<misc::DateTime>(current_time_stamp);
    }
    else
    {
        should_recompile = true;
    }


    D3DDataBlob shader_byte_code{};

    if(!should_recompile)
    {
        // Attempt to use cached version of the shader

        SharedDataChunk blob = p_shader_cache_containing_requested_shader->retrieveEntry(shader_cache_key);
        if (!blob.data())
        {
            LEXGINE_LOG_ERROR(this, "Unable to retrieve precompiled shader byte code for source \""
                + m_source_name + "\"");
            should_recompile = true;
        }
        else
        {
            Microsoft::WRL::ComPtr<ID3DBlob> d3d_blob{ nullptr };
            HRESULT hres = D3DCreateBlob(blob.size(), d3d_blob.GetAddressOf());
            if (hres != S_OK && hres != S_FALSE)
            {
                LEXGINE_LOG_ERROR(this, "Unable to create D3D blob to store precompiled shader code for source \""
                    + m_source_name + "\"");
                should_recompile = true;
            }
            else
            {
                memcpy(d3d_blob->GetBufferPointer(), blob.data(), blob.size());
                shader_byte_code = D3DDataBlob{ d3d_blob };
            }
        }
    }

    if (should_recompile)
    {
        // Need to recompile the shader

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
                D3DCompile(processed_shader_source.c_str(), processed_shader_source.size(),
                    m_source_name.c_str(), macro_definitions.data(), NULL, m_shader_entry_point.c_str(), target.c_str(),
                    compilation_flags, NULL, p_shader_bytecode_blob.GetAddressOf(), p_compilation_errors_blob.GetAddressOf()),
                S_OK);

            if (p_compilation_errors_blob)
            {
                m_compilation_log = std::string{ static_cast<char const*>(p_compilation_errors_blob->GetBufferPointer()) };
                std::string output_log = "Unable to compile shader source located in \"" + full_shader_path + "\". "
                    "Detailed compiler log follows: <em>" + m_compilation_log + "</em>";
                LEXGINE_LOG_ERROR(this, output_log);
                m_was_compilation_successful = false;
            }
            else
            {
                shader_byte_code = D3DDataBlob{ p_shader_bytecode_blob };
                m_was_compilation_successful = true;
            }

        }
        else
        {
            // use LLVM-based compiler with SM6 support

            if (!m_dxc_proxy.compile(worker_id, processed_shader_source, m_source_name, m_shader_entry_point, target,
                m_preprocessor_macro_definitions, m_optimization_level, m_is_strict_mode_enabled,
                m_is_all_resources_binding_forced, m_is_ieee_forced, m_are_warnings_treated_as_errors,
                m_is_validation_enabled, m_should_enable_debug_information, m_should_enable_16bit_types))
            {
                LEXGINE_LOG_ERROR(this, "Compilation of HLSL source \"" + m_source_name 
                    + "\" has failed (details: " + m_dxc_proxy.errors(worker_id) + ")");
                m_was_compilation_successful = false;
            }
            else
            {
                auto compilation_result = m_dxc_proxy.result(worker_id);
                if (compilation_result.isValid())
                {
                    shader_byte_code = static_cast<D3DDataBlob>(compilation_result);
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
            shader_caches.front().addEntry(shader_cache_type::entry_type{ shader_cache_key, shader_byte_code });
        }

    }

   

    if (m_type == ShaderType::compute)
    {
        ComputePSODescriptor* p_compute_pso_descriptors = static_cast<ComputePSODescriptor*>(m_p_target_pso_descriptors);

        for (uint32_t i = 0; i < m_num_target_descriptors; ++i)
            p_compute_pso_descriptors[i].compute_shader = shader_byte_code;
    }
    else
    {
        GraphicsPSODescriptor* p_graphics_pso_descriptors = static_cast<GraphicsPSODescriptor*>(m_p_target_pso_descriptors);

        for (uint32_t i = 0; i < m_num_target_descriptors; ++i)
        {
            switch (m_type)
            {
            case ShaderType::vertex:
                p_graphics_pso_descriptors[i].vertex_shader = shader_byte_code;
                break;
            case ShaderType::hull:
                p_graphics_pso_descriptors[i].hull_shader = shader_byte_code;
                break;
            case ShaderType::domain:
                p_graphics_pso_descriptors[i].domain_shader = shader_byte_code;
                break;
            case ShaderType::geometry:
                p_graphics_pso_descriptors[i].geometry_shader = shader_byte_code;
                break;
            case ShaderType::pixel:
                p_graphics_pso_descriptors[i].pixel_shader = shader_byte_code;
                break;
            }
        }
    }

    return true;
}

lexgine::core::concurrency::TaskType HLSLCompilationTask::get_task_type() const
{
    return concurrency::TaskType::cpu;
}


