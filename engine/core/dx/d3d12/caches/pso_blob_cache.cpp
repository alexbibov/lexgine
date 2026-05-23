#include "utility"
#include "tuple"

#include "d3dcompiler.h"

#include "engine/core/globals.h"
#include "engine/core/global_settings.h"
#include "engine/core/exception.h"
#include "engine/core/gpu_data_blob_cache.h"
#include "engine/core/misc/hashes/blake3_256.h"
#include "engine/core/dx/dxgi/hw_adapter_enumerator.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/d3d_data_blob.h"

#include "pso_blob_cache.h"

namespace lexgine::core::dx::d3d12::caches {

namespace {

SharedDataChunk makeSharedDataChunk(DataBlob const& blob)
{
    if (!blob.data() || !blob.size()) return {};
    SharedDataChunk chunk { blob.size() };
    memcpy(chunk.data(), blob.data(), blob.size());
    return chunk;
}

D3DDataBlob loadPrecachedPSOBlob(GpuDataBlobCache& cache, GpuDataBlobCacheKey const& key,
    misc::DateTime const& timestamp)
{
    D3DDataBlob rv { nullptr };
    if (!cache) return rv;

    SharedDataChunk cached_pso_blob = cache.find(key, timestamp);
    if (!cached_pso_blob.size() || !cached_pso_blob.data()) return rv;

    Microsoft::WRL::ComPtr<ID3DBlob> d3d_blob { nullptr };
    HRESULT res = D3DCreateBlob(cached_pso_blob.size(), d3d_blob.GetAddressOf());
    if (SUCCEEDED(res))
    {
        memcpy(d3d_blob->GetBufferPointer(), cached_pso_blob.data(), cached_pso_blob.size());
        rv = D3DDataBlob { d3d_blob };
    }
    return rv;
}

misc::hashes::Blake3_256 combinedHash(
    misc::HashValue const& descriptor_hash,
    RootSignatureHandle rs_handle)
{
    misc::hashes::Blake3_256 h {};
    h.create(descriptor_hash.hashValue(), descriptor_hash.hashWidth());
    if (rs_handle.p_internal)
    {
        uint8_t rs_bytes[GpuDataBlobCacheKey::serialized_size];
        rs_handle.p_internal->serialize(rs_bytes);
        h.combine(rs_bytes, sizeof(rs_bytes));
    }
    else
    {
        uint8_t null_marker = 0;
        h.combine(&null_marker, sizeof(null_marker));
    }
    h.finalize();
    return h;
}

}

PSOBlobCache::PSOBlobCache(Globals& globals)
    : m_globals { globals }
    , m_device { *globals.get<Device>() }
    , m_gpu_blob_cache { *globals.get<GpuDataBlobCache>() }
    , m_rs_blob_cache { *globals.get<RootSignatureBlobCache>() }
    , m_async_pso_creation { globals.get<GlobalSettings>()->isDeferredGpuResourceCompilationOn() }
{
}

PSOBlobCache::~PSOBlobCache()
{
    waitTillReady();
}

GraphicsPSOHandle PSOBlobCache::createGraphicsPSOBlobCompilationContract(
    GraphicsPSODescriptor const& descriptor,
    RootSignatureHandle root_signature_handle,
    misc::DateTime const& timestamp)
{
    std::unique_lock l { m_lock };
    waitTillReady();
    GpuDataBlobCacheKey key = createGraphicsGpuDataBlobCacheKey(descriptor, root_signature_handle);
    auto cit = m_graphics_contracts.find(key);
    if (cit != m_graphics_contracts.end())
    {
        return { &cit->first };
    }
    auto [ncit, _] = m_graphics_contracts.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::move(key)),
        std::forward_as_tuple(
            descriptor, root_signature_handle, timestamp,
            std::bind(&PSOBlobCache::compileGraphicsPSOBlob, this, std::placeholders::_1))
    );
    m_cached_graphics.emplace(
        std::make_pair(
            GraphicsPSOHandle { .p_internal = &ncit->first },
            GraphicsResult {
                .p_contract = &ncit->second,
                .future = ncit->second.task.get_future(),
                .pso = nullptr }));
    return { &ncit->first };
}

ComputePSOHandle PSOBlobCache::createComputePSOBlobCompilationContract(
    ComputePSODescriptor const& descriptor,
    RootSignatureHandle root_signature_handle,
    misc::DateTime const& timestamp)
{
    std::unique_lock l { m_lock };
    waitTillReady();
    GpuDataBlobCacheKey key = createComputeGpuDataBlobCacheKey(descriptor, root_signature_handle);
    auto cit = m_compute_contracts.find(key);
    if (cit != m_compute_contracts.end())
    {
        return { &cit->first };
    }
    auto [ncit, _] = m_compute_contracts.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::move(key)),
        std::forward_as_tuple(
            descriptor, root_signature_handle, timestamp,
            std::bind(&PSOBlobCache::compileComputePSOBlob, this, std::placeholders::_1))
    );
    m_cached_compute.emplace(
        std::make_pair(
            ComputePSOHandle { .p_internal = &ncit->first },
            ComputeResult {
                .p_contract = &ncit->second,
                .future = ncit->second.task.get_future(),
                .pso = nullptr }));
    return { &ncit->first };
}

void PSOBlobCache::createPipelineStates()
{
    std::unique_lock l { m_lock };
    waitTillReady();
    m_unresolved_graphics.clear();
    m_unresolved_compute.clear();
    m_unresolved_graphics.reserve(m_graphics_contracts.size());
    m_unresolved_compute.reserve(m_compute_contracts.size());
    for (auto& [k, c] : m_graphics_contracts)
    {
        if (c.status.load() == PSOBlobCompilationStatus::NotStarted)
        {
            m_unresolved_graphics.push_back(std::make_pair(GraphicsPSOHandle { &k }, &c));
        }
    }
    for (auto& [k, c] : m_compute_contracts)
    {
        if (c.status.load() == PSOBlobCompilationStatus::NotStarted)
        {
            m_unresolved_compute.push_back(std::make_pair(ComputePSOHandle { &k }, &c));
        }
    }
    size_t total = m_unresolved_graphics.size() + m_unresolved_compute.size();
    if (total == 0) return;

    size_t num_threads = m_globals.get<GlobalSettings>()->getNumberOfWorkers();
    if (m_async_pso_creation && num_threads > 0)
    {
        size_t per_bucket_count = total / num_threads;
        size_t rem = total % num_threads;
        if (per_bucket_count == 0)
        {
            num_threads = rem;
        }
        m_worker_threads.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i)
        {
            size_t start = per_bucket_count * i + (i < rem ? i : rem);
            size_t count = per_bucket_count + (i < rem ? 1 : 0);
            m_worker_threads.emplace_back(
                std::thread {
                    [this](size_t s, size_t cnt) {
                        size_t gfx_count = m_unresolved_graphics.size();
                        for (size_t j = s; j < s + cnt; ++j)
                        {
                            if (j < gfx_count)
                            {
                                auto [p_key, p_contract] = m_unresolved_graphics[j];
                                p_contract->task(p_key);
                            }
                            else
                            {
                                auto [p_key, p_contract] = m_unresolved_compute[j - gfx_count];
                                p_contract->task(p_key);
                            }
                        }
                    },
                    start,
                    count
                }
            );
        }
    }
    else
    {
        for (auto& [p_key, p_contract] : m_unresolved_graphics)
        {
            p_contract->task(p_key);
        }
        for (auto& [p_key, p_contract] : m_unresolved_compute)
        {
            p_contract->task(p_key);
        }
    }
}

PipelineState const* PSOBlobCache::getGraphicsPipelineState(GraphicsPSOHandle handle) const
{
    std::unique_lock l { m_lock };
    auto it = m_cached_graphics.find(handle);
    if (it == m_cached_graphics.end()) return nullptr;
    if (it->second.pso) return it->second.pso.get();
    GraphicsContract const& contract = *it->second.p_contract;
    if (contract.status.load() != PSOBlobCompilationStatus::Completed) return nullptr;
    it->second.pso = it->second.future.get();
    return it->second.pso.get();
}

PipelineState const* PSOBlobCache::getComputePipelineState(ComputePSOHandle handle) const
{
    std::unique_lock l { m_lock };
    auto it = m_cached_compute.find(handle);
    if (it == m_cached_compute.end()) return nullptr;
    if (it->second.pso) return it->second.pso.get();
    ComputeContract const& contract = *it->second.p_contract;
    if (contract.status.load() != PSOBlobCompilationStatus::Completed) return nullptr;
    it->second.pso = it->second.future.get();
    return it->second.pso.get();
}

std::unique_ptr<PipelineState> PSOBlobCache::compileGraphicsPSOBlob(GraphicsPSOHandle handle)
{
    assert(handle.p_internal);

    auto cit = m_graphics_contracts.find(*handle.p_internal);
    assert(cit != m_graphics_contracts.end());

    cit->second.status.store(PSOBlobCompilationStatus::Failed);

    GraphicsContract& contract = cit->second;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> native_rs =
        m_rs_blob_cache.getNativeRootSignature(contract.rs_handle);
    if (!native_rs) return nullptr;

    D3DDataBlob precached_pso_blob = loadPrecachedPSOBlob(m_gpu_blob_cache, *handle.p_internal, contract.timestamp);

    std::unique_ptr<PipelineState> pso;
    try
    {
        pso = std::make_unique<PipelineState>(m_globals, native_rs, contract.descriptor, precached_pso_blob);
    }
    catch (Exception const& e)
    {
        LEXGINE_LOG_ERROR(this, std::string { "Failed to compile graphics PSO: " } + e.what());
        return nullptr;
    }
    catch (...)
    {
        LEXGINE_LOG_ERROR(this, "Failed to compile graphics PSO (unspecified exception)");
        return nullptr;
    }

    if (!precached_pso_blob && m_gpu_blob_cache)
    {
        m_gpu_blob_cache.put(*handle.p_internal, makeSharedDataChunk(pso->getCache()));
    }

    cit->second.status.store(PSOBlobCompilationStatus::Completed);
    return pso;
}

std::unique_ptr<PipelineState> PSOBlobCache::compileComputePSOBlob(ComputePSOHandle handle)
{
    assert(handle.p_internal);

    auto cit = m_compute_contracts.find(*handle.p_internal);
    assert(cit != m_compute_contracts.end());

    cit->second.status.store(PSOBlobCompilationStatus::Failed);

    ComputeContract& contract = cit->second;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> native_rs =
        m_rs_blob_cache.getNativeRootSignature(contract.rs_handle);
    if (!native_rs) return nullptr;

    D3DDataBlob precached_pso_blob = loadPrecachedPSOBlob(m_gpu_blob_cache, *handle.p_internal, contract.timestamp);

    std::unique_ptr<PipelineState> pso;
    try
    {
        pso = std::make_unique<PipelineState>(m_globals, native_rs, contract.descriptor, precached_pso_blob);
    }
    catch (Exception const& e)
    {
        LEXGINE_LOG_ERROR(this, std::string { "Failed to compile compute PSO: " } + e.what());
        return nullptr;
    }
    catch (...)
    {
        LEXGINE_LOG_ERROR(this, "Failed to compile compute PSO (unspecified exception)");
        return nullptr;
    }

    if (!precached_pso_blob && m_gpu_blob_cache)
    {
        m_gpu_blob_cache.put(*handle.p_internal, makeSharedDataChunk(pso->getCache()));
    }

    cit->second.status.store(PSOBlobCompilationStatus::Completed);
    return pso;
}

GpuDataBlobCacheKey PSOBlobCache::createGraphicsGpuDataBlobCacheKey(
    GraphicsPSODescriptor const& descriptor,
    RootSignatureHandle rs_handle)
{
    LUID adapter_luid = m_device.hwAdapter()->getProperties().details.luid;
    misc::UUID gpu_driver_uuid {
        static_cast<uint64_t>(adapter_luid.LowPart),
        static_cast<uint64_t>(adapter_luid.HighPart)
    };
    misc::hashes::Blake3_256 combined = combinedHash(*descriptor.hash(), rs_handle);
    GpuDataBlobCacheKey::CommonManifest manifest = GpuDataBlobCacheKey::createManifest(
        { 'P', 'S', 'O', 'G' },
        gpu_driver_uuid,
        combined
    );
    InternalKey internal_key {};
    internal_key.pso_format_version = c_pso_format_version;
    return GpuDataBlobCacheKey { manifest, internal_key };
}

GpuDataBlobCacheKey PSOBlobCache::createComputeGpuDataBlobCacheKey(
    ComputePSODescriptor const& descriptor,
    RootSignatureHandle rs_handle)
{
    LUID adapter_luid = m_device.hwAdapter()->getProperties().details.luid;
    misc::UUID gpu_driver_uuid {
        static_cast<uint64_t>(adapter_luid.LowPart),
        static_cast<uint64_t>(adapter_luid.HighPart)
    };
    misc::hashes::Blake3_256 combined = combinedHash(*descriptor.hash(), rs_handle);
    GpuDataBlobCacheKey::CommonManifest manifest = GpuDataBlobCacheKey::createManifest(
        { 'P', 'S', 'O', 'C' },
        gpu_driver_uuid,
        combined
    );
    InternalKey internal_key {};
    internal_key.pso_format_version = c_pso_format_version;
    return GpuDataBlobCacheKey { manifest, internal_key };
}

void PSOBlobCache::waitTillReady()
{
    std::unique_lock l { m_lock };
    for (std::thread& t : m_worker_threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    m_worker_threads.clear();
}

}
