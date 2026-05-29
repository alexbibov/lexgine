#include <chrono>
#include <cstring>
#include <tuple>
#include <utility>

#include "d3dcompiler.h"

#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/dxcompilation/dx_compiler_proxy.h"
#include "engine/core/exception.h"
#include "engine/core/global_settings.h"
#include "engine/core/globals.h"
#include "engine/core/gpu_data_blob_cache.h"
#include "engine/core/misc/misc.h"
#include "engine/core/misc/hashes/blake3_256.h"

#include "hlsl_shader_blob_cache.h"

namespace lexgine::core::dx::d3d12::caches {

namespace {

SharedDataChunk makeSharedDataChunk(DataBlob const& blob)
{
    if (!blob.data() || !blob.size()) return {};

    SharedDataChunk chunk { blob.size() };
    memcpy(chunk.data(), blob.data(), blob.size());
    return chunk;
}

D3DDataBlob loadPrecachedShaderBlob(GpuDataBlobCache& cache, GpuDataBlobCacheKey const& key, misc::DateTime const& timestamp)
{
    D3DDataBlob rv { nullptr };
    if (!cache) return rv;

    SharedDataChunk cached_shader_blob = cache.find(key, timestamp);
    if (!cached_shader_blob.size() || !cached_shader_blob.data()) return rv;

    Microsoft::WRL::ComPtr<ID3DBlob> d3d_blob { nullptr };
    HRESULT hres = D3DCreateBlob(cached_shader_blob.size(), d3d_blob.GetAddressOf());
    if (SUCCEEDED(hres))
    {
        memcpy(d3d_blob->GetBufferPointer(), cached_shader_blob.data(), cached_shader_blob.size());
        rv = D3DDataBlob { d3d_blob };
    }
    return rv;
}

std::string getStringifiedDefines(std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions)
{
    std::string rv {};
    rv += "DEFINES={";
    for (auto const& def : macro_definitions)
        rv += "{NAME=" + def.name + ", VALUE=" + def.value + "}, ";
    rv += "}";
    return rv;
}

misc::hashes::Blake3_256 combineHashValue(
    misc::HashValue const& translation_unit_hash, 
    std::string const& shader_entry_point,
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions
)
{
    misc::hashes::Blake3_256 h{};
    std::string stringified_defines = getStringifiedDefines(macro_definitions);
    h.create(translation_unit_hash.hashValue(), translation_unit_hash.hashWidth());
    h.combine(shader_entry_point.data(), shader_entry_point.size());
    h.combine(stringified_defines.data(), stringified_defines.size());
    h.finalize();
    return h;
}

}  // namespace

misc::HashValue const* HLSLTranslationUnit::hash() const
{
    if (m_hash_value) return m_hash_value.get();
    auto h = std::make_unique<misc::hashes::Blake3_256>();
    h->create(m_hlsl_source_code.data(), m_hlsl_source_code.size());
    h->combine(m_source_name.data(), m_source_name.size());
    h->finalize();
    m_hash_value = std::move(h);
    return m_hash_value.get();
}

HLSLFileTranslationUnit::HLSLFileTranslationUnit(Globals& globals, std::string const& source_name, std::filesystem::path const& file_path)
    : HLSLTranslationUnit { globals }
{
    m_source_name = source_name;

    auto& global_settings = *globals.get<GlobalSettings>();
    auto const& shader_lookup_directories = global_settings.getShaderLookupDirectories();

    bool shader_found { false };
    for (auto const& path_prefix : shader_lookup_directories)
    {
        m_path_to_shader = path_prefix / file_path;
        if (std::filesystem::exists(m_path_to_shader))
        {
            shader_found = true;
            break;
        }
    }

    if (!shader_found)
    {
        LEXGINE_THROW_ERROR("Unable to retrieve shader asset \"" + file_path.string() + "\"");
    }

    m_hlsl_source_code = ShaderSourceCodePreprocessor {
        m_path_to_shader.string(),
        ShaderSourceCodePreprocessor::SourceType::file,
        shader_lookup_directories
    }.getPreprocessedSource();
    m_timestamp = *misc::getFileLastUpdatedTimeStamp(m_path_to_shader.string());
}

misc::HashValue const* HLSLFileTranslationUnit::hash() const
{
    if (m_hash_value) return m_hash_value.get();
    auto h = std::make_unique<misc::hashes::Blake3_256>();
    h->create(m_hlsl_source_code.data(), m_hlsl_source_code.size());
    h->combine(m_source_name.data(), m_source_name.size());
    std::string p = m_path_to_shader.string();
    h->combine(p.data(), p.size());
    h->finalize();
    m_hash_value = std::move(h);
    return m_hash_value.get();
}

HLSLShaderBlobCache::ShaderContract::ShaderContract(
    misc::DateTime const& timestamp,
    std::string const& hlsl_source,
    std::string const& source_name,
    dxcompilation::ShaderModel shader_model,
    dxcompilation::ShaderType shader_type,
    std::string const& shader_entry_point,
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions,
    dxcompilation::HLSLCompilationOptimizationLevel optimization_level,
    bool strict_mode,
    bool force_all_resources_be_bound,
    bool force_ieee_standard,
    bool treat_warnings_as_errors,
    bool enable_validation,
    bool enable_debug_information,
    bool enable_16bit_types,
    std::function<D3DDataBlob(HLSLShaderHandle, uint8_t)> const& op)
    : timestamp { timestamp }
    , hlsl_source { hlsl_source }
    , source_name { source_name }
    , shader_model { shader_model }
    , shader_type { shader_type }
    , shader_entry_point { shader_entry_point }
    , macro_definitions { macro_definitions }
    , optimization_level { optimization_level }
    , strict_mode { strict_mode }
    , force_all_resources_be_bound { force_all_resources_be_bound }
    , force_ieee_standard { force_ieee_standard }
    , treat_warnings_as_errors { treat_warnings_as_errors }
    , enable_validation { enable_validation }
    , enable_debug_information { enable_debug_information }
    , enable_16bit_types { enable_16bit_types }
    , task { op }
    , status { HLSLShaderBlobCompilationStatus::NotStarted }
{
    std::pair<uint8_t, uint8_t> shader_model_version = HLSLShaderBlobCache::unpackShaderModelVersion(this->shader_model);

    if (shader_model_version.first < 6)
    {
        if (this->enable_16bit_types)
        {
            misc::Log::retrieve()->out("Legacy HLSL compiler does not support 16-bit reduced precision types. "
                "This capability will be forced to \"false\"", misc::LogMessageType::exclamation);
            this->enable_16bit_types = false;
        }
    }
    else if (shader_model_version.first == 6 && shader_model_version.second < 2)
    {
        if (this->enable_16bit_types)
        {
            misc::Log::retrieve()->out("Reduced precision 16-bit types require shader model 6.2 although shader model "
                + std::to_string(shader_model_version.first) + "." + std::to_string(shader_model_version.second)
                + " was requested. The shader model will be forced to 6.2", misc::LogMessageType::exclamation);
            this->shader_model = dxcompilation::ShaderModel::model_62;
        }
    }

    if (this->enable_debug_information)
    {
        this->optimization_level = dxcompilation::HLSLCompilationOptimizationLevel::level_no;
        this->enable_validation = true;
    }
}

std::pair<uint8_t, uint8_t> HLSLShaderBlobCache::unpackShaderModelVersion(dxcompilation::ShaderModel shader_model)
{
    uint8_t version_major = static_cast<uint8_t>((static_cast<unsigned short>(shader_model) >> 4) & 0xF);
    uint8_t version_minor = static_cast<uint8_t>(static_cast<unsigned short>(shader_model) & 0xF);
    return std::make_pair(version_major, version_minor);
}

std::string HLSLShaderBlobCache::shaderModelAndTypeToTargetName(dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type)
{
    char target[7] = { 0 };
    char const* prefix = nullptr;

    std::pair<uint8_t, uint8_t> shader_model_version = unpackShaderModelVersion(shader_model);

    switch (shader_type)
    {
    case dxcompilation::ShaderType::vertex:
        prefix = "vs_";
        break;
    case dxcompilation::ShaderType::hull:
        prefix = "hs_";
        break;
    case dxcompilation::ShaderType::domain:
        prefix = "ds_";
        break;
    case dxcompilation::ShaderType::geometry:
        prefix = "gs_";
        break;
    case dxcompilation::ShaderType::pixel:
        prefix = "ps_";
        break;
    case dxcompilation::ShaderType::compute:
        prefix = "cs_";
        break;
    }
    assert(sizeof(target) >= strlen(prefix) + 3);
    memcpy(target, prefix, strlen(prefix));
    target[3] = '0' + shader_model_version.first;
    target[4] = '_';
    target[5] = '0' + shader_model_version.second;

    return target;
}

HLSLShaderBlobCache::HLSLShaderBlobCache(Globals& globals)
    : m_globals { globals }
    , m_gpu_blob_cache { *globals.get<GpuDataBlobCache>() }
    , m_async_shader_compilation { globals.get<GlobalSettings>()->isDeferredShaderCompilationOn() }
{
}

HLSLShaderBlobCache::~HLSLShaderBlobCache()
{
    waitTillReady();
}

HLSLShaderHandle HLSLShaderBlobCache::createHLSLShaderBlobCompilationContract(HLSLFileTranslationUnit const& hlsl_translation_unit,
    dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions,
    dxcompilation::HLSLCompilationOptimizationLevel optimization_level,
    bool strict_mode, bool force_all_resources_be_bound,
    bool force_ieee_standard, bool treat_warnings_as_errors, bool enable_validation,
    bool enable_debug_information, bool enable_16bit_types)
{
    return createHLSLShaderBlobCompilationContract(
        hlsl_translation_unit,
        shader_model,
        shader_type,
        shader_entry_point,
        macro_definitions,
        optimization_level,
        strict_mode,
        force_all_resources_be_bound,
        force_ieee_standard,
        treat_warnings_as_errors,
        enable_validation,
        enable_debug_information,
        enable_16bit_types);
}

HLSLShaderHandle HLSLShaderBlobCache::createHLSLShaderBlobCompilationContract(HLSLSourceTranslationUnit const& hlsl_translation_unit,
    dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions,
    dxcompilation::HLSLCompilationOptimizationLevel optimization_level,
    bool strict_mode, bool force_all_resources_be_bound,
    bool force_ieee_standard, bool treat_warnings_as_errors, bool enable_validation,
    bool enable_debug_information, bool enable_16bit_types)
{
    return createHLSLShaderBlobCompilationContract(
        hlsl_translation_unit,
        shader_model,
        shader_type,
        shader_entry_point,
        macro_definitions,
        optimization_level,
        strict_mode,
        force_all_resources_be_bound,
        force_ieee_standard,
        treat_warnings_as_errors,
        enable_validation,
        enable_debug_information,
        enable_16bit_types);
}

HLSLShaderHandle HLSLShaderBlobCache::createHLSLShaderBlobCompilationContract(
    HLSLTranslationUnit const& hlsl_translation_unit,
    dxcompilation::ShaderModel shader_model,
    dxcompilation::ShaderType shader_type,
    std::string const& shader_entry_point,
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions,
    dxcompilation::HLSLCompilationOptimizationLevel optimization_level,
    bool strict_mode,
    bool force_all_resources_be_bound,
    bool force_ieee_standard,
    bool treat_warnings_as_errors,
    bool enable_validation,
    bool enable_debug_information,
    bool enable_16bit_types)
{
    std::unique_lock l { m_lock };
    waitTillReady();
   
    GpuDataBlobCacheKey key = createGpuDataBlobCacheKey(
        hlsl_translation_unit,
        shader_model,
        shader_type,
        shader_entry_point,
        macro_definitions,
        optimization_level,
        strict_mode,
        force_all_resources_be_bound,
        force_ieee_standard,
        treat_warnings_as_errors,
        enable_validation,
        enable_debug_information,
        enable_16bit_types
    );

    auto cit = m_contracts.find(key);
    if (cit != m_contracts.end())
    {
        return { &cit->first };
    }

    auto [ncit, _] = m_contracts.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::move(key)),
        std::forward_as_tuple(
            hlsl_translation_unit.timestamp(),
            hlsl_translation_unit.source(),
            hlsl_translation_unit.name(),
            shader_model,
            shader_type,
            shader_entry_point,
            macro_definitions,
            optimization_level,
            strict_mode,
            force_all_resources_be_bound,
            force_ieee_standard,
            treat_warnings_as_errors,
            enable_validation,
            enable_debug_information,
            enable_16bit_types,
            std::bind(&HLSLShaderBlobCache::compileShaderBlob, this, std::placeholders::_1, std::placeholders::_2))
    );

    m_cached_shaders.emplace(
        std::make_pair(
            HLSLShaderHandle { .p_internal = &ncit->first },
            ShaderResult {
                .p_contract = &ncit->second,
                .future = ncit->second.task.get_future(),
                .shader_blob = nullptr }));

    return { &ncit->first };
}

GpuDataBlobCacheKey HLSLShaderBlobCache::createGpuDataBlobCacheKey(
    HLSLTranslationUnit const& hlsl_translation_unit,
    dxcompilation::ShaderModel shader_model,
    dxcompilation::ShaderType shader_type,
    std::string const& shader_entry_point,
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions,
    dxcompilation::HLSLCompilationOptimizationLevel optimization_level,
    bool strict_mode,
    bool force_all_resources_be_bound,
    bool force_ieee_standard,
    bool treat_warnings_as_errors,
    bool enable_validation,
    bool enable_debug_information,
    bool enable_16bit_types
) const
{
    misc::hashes::Blake3_256 h{};
    {
        misc::HashValue const* p_translation_unit_hash = hlsl_translation_unit.hash();
        h = combineHashValue(*p_translation_unit_hash, shader_entry_point, macro_definitions);
    }
    misc::UUID gpu_driver_uuid{};
    GpuDataBlobCacheKey::CommonManifest manifest = GpuDataBlobCacheKey::createManifest(
        { 'H', 'L', 'S', 'L' },
        gpu_driver_uuid,
        h
    );
    InternalKey internal_key{};
    internal_key.shader_type = shader_type;
    internal_key.shader_model = shader_model;
    internal_key.optimization_level = optimization_level;
    internal_key.strict_mode = strict_mode;
    internal_key.force_all_resources_be_bound = force_all_resources_be_bound;
    internal_key.force_ieee_standard = force_ieee_standard;

    return GpuDataBlobCacheKey{ manifest, internal_key };
}

void HLSLShaderBlobCache::createShaderBlobs()
{
    std::unique_lock l { m_lock };
    waitTillReady();

    m_unresolved_contracts.clear();
    m_unresolved_contracts.reserve(m_contracts.size());
    for (auto& [k, c] : m_contracts)
    {
        if (c.status.load() == HLSLShaderBlobCompilationStatus::NotStarted)
        {
            m_unresolved_contracts.push_back(std::make_pair(HLSLShaderHandle { &k }, &c));
        }
    }
    if (m_unresolved_contracts.empty()) return;

    size_t num_threads = m_globals.get<GlobalSettings>()->getNumberOfWorkers();
    if (m_async_shader_compilation && num_threads > 0)
    {
        size_t per_bucket_count = m_unresolved_contracts.size() / num_threads;
        size_t rem = m_unresolved_contracts.size() % num_threads;
        if (per_bucket_count == 0)
        {
            num_threads = rem;
        }
        m_worker_threads.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i)
        {
            m_worker_threads.emplace_back(
                std::thread {
                    [this](size_t start, size_t count, uint8_t worker_id)
                    {
                        for (size_t j = start; j < start + count; ++j)
                        {
                            auto [handle, p_contract] = m_unresolved_contracts[j];
                            HLSLShaderBlobCompilationStatus expected = HLSLShaderBlobCompilationStatus::NotStarted;
                            if (p_contract->status.compare_exchange_strong(expected, HLSLShaderBlobCompilationStatus::Started))
                            {
                                p_contract->task(handle, worker_id);
                            }
                        }
                    },
                    per_bucket_count * i + (i < rem ? i : rem),
                    per_bucket_count + (i < rem ? 1 : 0),
                    static_cast<uint8_t>(i)
                }
            );
        }
    }
    else
    {
        for (auto& [handle, p_contract] : m_unresolved_contracts)
        {
            p_contract->status.store(HLSLShaderBlobCompilationStatus::Started);
            p_contract->task(handle, 0);
        }
    }
}

void HLSLShaderBlobCache::waitTillReady()
{
	std::unique_lock l{ m_lock };
	for (std::thread& t : m_worker_threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}
	m_worker_threads.clear();
}

std::pair<D3DDataBlob, HLSLShaderBlobCompilationStatus> HLSLShaderBlobCache::getShaderBlob(HLSLShaderHandle handle) const
{
    std::unique_lock l { m_lock };
    auto it = m_cached_shaders.find(handle);
    if (it == m_cached_shaders.end())
        return std::make_pair(D3DDataBlob { nullptr }, HLSLShaderBlobCompilationStatus::NotScheduled);
    if (it->second.shader_blob)
        return std::make_pair(it->second.shader_blob, HLSLShaderBlobCompilationStatus::Completed);

    ShaderContract& contract = *it->second.p_contract;
    HLSLShaderBlobCompilationStatus status = contract.status.load();
    if (status == HLSLShaderBlobCompilationStatus::NotStarted)
    {
        HLSLShaderBlobCompilationStatus expected = HLSLShaderBlobCompilationStatus::NotStarted;
        if (contract.status.compare_exchange_strong(expected, HLSLShaderBlobCompilationStatus::Started))
        {
            contract.task(handle, 0);
            status = contract.status.load();
        }
        else
        {
            status = expected;
        }
    }
    if (status == HLSLShaderBlobCompilationStatus::Started)
    {
        GlobalSettings const& global_settings = *m_globals.get<GlobalSettings>();
        uint32_t timeout = global_settings.getMaxNonBlockingUploadBufferAllocationTimeout();
        if (it->second.future.wait_for(std::chrono::milliseconds { timeout }) != std::future_status::ready)
        {
            return std::make_pair(D3DDataBlob { nullptr }, HLSLShaderBlobCompilationStatus::Started);
        }
        status = contract.status.load();
    }
    if (status == HLSLShaderBlobCompilationStatus::Failed)
    {
        return std::make_pair(D3DDataBlob { nullptr }, HLSLShaderBlobCompilationStatus::Failed);
    }

    it->second.shader_blob = it->second.future.get();
    return std::make_pair(it->second.shader_blob, contract.status.load());
}

dxcompilation::ShaderType HLSLShaderBlobCache::getShaderType(HLSLShaderHandle handle) const
{
    std::unique_lock l { m_lock };
    auto it = m_cached_shaders.find(handle);
    assert(it != m_cached_shaders.end());
    return it->second.p_contract->shader_type;
}

dxcompilation::ShaderModel HLSLShaderBlobCache::getShaderModel(HLSLShaderHandle handle) const
{
    std::unique_lock l { m_lock };
    auto it = m_cached_shaders.find(handle);
    assert(it != m_cached_shaders.end());
    return it->second.p_contract->shader_model;
}

std::string HLSLShaderBlobCache::getShaderCacheName(HLSLShaderHandle handle) const
{
    assert(handle.p_internal);
    return handle.p_internal->toString();
}

std::string HLSLShaderBlobCache::getCompilationLog(HLSLShaderHandle handle) const
{
    std::unique_lock l { m_lock };
    auto it = m_cached_shaders.find(handle);
    assert(it != m_cached_shaders.end());
    return it->second.p_contract->compilation_log;
}

bool HLSLShaderBlobCache::isPrecached(HLSLShaderHandle handle) const
{
    std::unique_lock l { m_lock };
    auto it = m_cached_shaders.find(handle);
    assert(it != m_cached_shaders.end());
    return it->second.p_contract->precached;
}

D3DDataBlob HLSLShaderBlobCache::compileShaderBlob(HLSLShaderHandle handle, uint8_t worker_id)
{
    assert(handle.p_internal);

    auto cit = m_contracts.find(*handle.p_internal);
    assert(cit != m_contracts.end());

    try
    {
        ShaderContract& contract = cit->second;
        D3DDataBlob shader_byte_code = loadPrecachedShaderBlob(m_gpu_blob_cache, *handle.p_internal, contract.timestamp);
        if (shader_byte_code)
        {
            contract.precached = true;
            contract.status.store(HLSLShaderBlobCompilationStatus::Completed);
            return shader_byte_code;
        }

        std::string target = shaderModelAndTypeToTargetName(contract.shader_model, contract.shader_type);

        if (static_cast<unsigned short>(contract.shader_model) < static_cast<unsigned short>(dxcompilation::ShaderModel::model_60))
        {
            Microsoft::WRL::ComPtr<ID3DBlob> p_shader_bytecode_blob { nullptr };
            Microsoft::WRL::ComPtr<ID3DBlob> p_compilation_errors_blob { nullptr };

            std::vector<D3D_SHADER_MACRO> macro_definitions {};
            macro_definitions.resize(contract.macro_definitions.size() + 1);
            macro_definitions[contract.macro_definitions.size()] = D3D_SHADER_MACRO { NULL, NULL };
            uint32_t i = 0;
            for (auto p = contract.macro_definitions.begin(); p != contract.macro_definitions.end(); ++p, ++i)
            {
                macro_definitions[i].Name = p->name.c_str();
                macro_definitions[i].Definition = p->value.c_str();
            }

            UINT compilation_flags =
                D3DCOMPILE_AVOID_FLOW_CONTROL
                | (target[0] == 'c' ? D3DCOMPILE_RESOURCES_MAY_ALIAS : 0U)
                | D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES
                | (contract.strict_mode ? D3DCOMPILE_ENABLE_STRICTNESS : 0U)
                | (contract.force_all_resources_be_bound ? D3DCOMPILE_ALL_RESOURCES_BOUND : 0U)
                | (contract.force_ieee_standard ? D3DCOMPILE_IEEE_STRICTNESS : 0U)
                | (contract.treat_warnings_as_errors ? D3DCOMPILE_WARNINGS_ARE_ERRORS : 0U)
                | (contract.enable_validation ? 0U : D3DCOMPILE_SKIP_VALIDATION)
                | (contract.enable_debug_information ? D3DCOMPILE_DEBUG : 0U);

            switch (contract.optimization_level)
            {
            case dxcompilation::HLSLCompilationOptimizationLevel::level_no:
                compilation_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
                break;
            case dxcompilation::HLSLCompilationOptimizationLevel::level0:
                compilation_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
                break;
            case dxcompilation::HLSLCompilationOptimizationLevel::level1:
                compilation_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
                break;
            case dxcompilation::HLSLCompilationOptimizationLevel::level2:
                compilation_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL2;
                break;
            case dxcompilation::HLSLCompilationOptimizationLevel::level3:
                compilation_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
                break;
            }

            HRESULT hres = D3DCompile(contract.hlsl_source.c_str(), contract.hlsl_source.size(),
                contract.source_name.c_str(), macro_definitions.data(), NULL, contract.shader_entry_point.c_str(), target.c_str(),
                compilation_flags, NULL, p_shader_bytecode_blob.GetAddressOf(), p_compilation_errors_blob.GetAddressOf());

            if (p_compilation_errors_blob)
            {
                contract.compilation_log = std::string { static_cast<char const*>(p_compilation_errors_blob->GetBufferPointer()) };
            }

            if (FAILED(hres) || p_compilation_errors_blob)
            {
                std::string output_log = "Unable to compile shader source \"" + contract.source_name + "\". "
                    "Detailed compiler log follows: <em>" + contract.compilation_log + "</em>";
                LEXGINE_LOG_ERROR(this, output_log);
                cit->second.status.store(HLSLShaderBlobCompilationStatus::Failed);
                return nullptr;
            }

            shader_byte_code = D3DDataBlob { p_shader_bytecode_blob };
        }
        else
        {
            dxcompilation::DXCompilerProxy& dxc_proxy = m_globals.get<DxResourceFactory>()->shaderModel6xDxCompilerProxy();
            if (!dxc_proxy.compile(worker_id, contract.hlsl_source, contract.source_name, contract.shader_entry_point, target,
                contract.macro_definitions, contract.optimization_level, contract.strict_mode,
                contract.force_all_resources_be_bound, contract.force_ieee_standard, contract.treat_warnings_as_errors,
                contract.enable_validation, contract.enable_debug_information, contract.enable_16bit_types))
            {
                contract.compilation_log = dxc_proxy.errors(worker_id);
                LEXGINE_LOG_ERROR(this, "Compilation of HLSL source \"" + contract.source_name
                    + "\" has failed (details: <em>" + contract.compilation_log + "</em>)");
                cit->second.status.store(HLSLShaderBlobCompilationStatus::Failed);
                return nullptr;
            }

            auto compilation_result = dxc_proxy.result(worker_id);
            if (!compilation_result.isValid())
            {
                LEXGINE_LOG_ERROR(this, "Unable to retrieve DXIL byte code for shader source \"" + contract.source_name + "\"");
                cit->second.status.store(HLSLShaderBlobCompilationStatus::Failed);
                return nullptr;
            }

            shader_byte_code = *compilation_result;
        }

        if (shader_byte_code && m_gpu_blob_cache)
        {
            m_gpu_blob_cache.put(*handle.p_internal, makeSharedDataChunk(shader_byte_code));
        }

        cit->second.status.store(shader_byte_code ? HLSLShaderBlobCompilationStatus::Completed : HLSLShaderBlobCompilationStatus::Failed);
        return shader_byte_code;
    }
    catch (Exception const& e)
    {
        LEXGINE_LOG_ERROR(this, std::string { "Failed to compile HLSL shader: " } + e.what());
    }
    catch (...)
    {
        LEXGINE_LOG_ERROR(this, "Failed to compile HLSL shader: unknown exception");
    }

    cit->second.status.store(HLSLShaderBlobCompilationStatus::Failed);
    return nullptr;
}

}
