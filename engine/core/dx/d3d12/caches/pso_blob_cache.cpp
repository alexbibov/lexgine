#include <utility>
#include <tuple>
#include <chrono>

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

D3DDataBlob loadPrecachedPSOBlob(GpuDataBlobCache& cache, GpuDataBlobCacheKey const& key)
{
    D3DDataBlob rv { nullptr };
    if (!cache) return rv;

    SharedDataChunk cached_pso_blob = cache.find(key);
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
    RootSignatureHandle root_signature_handle)
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
            descriptor, root_signature_handle,
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
    RootSignatureHandle root_signature_handle)
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
            descriptor, root_signature_handle,
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
                                PSOBlobCompilationStatus expected = PSOBlobCompilationStatus::NotStarted;
                                if (p_contract->status.compare_exchange_strong(expected, PSOBlobCompilationStatus::Started))
                                {
                                    p_contract->task(p_key);
                                }
                            }
                            else
                            {
                                auto [p_key, p_contract] = m_unresolved_compute[j - gfx_count];
                                PSOBlobCompilationStatus expected = PSOBlobCompilationStatus::NotStarted;
                                if (p_contract->status.compare_exchange_strong(expected, PSOBlobCompilationStatus::Started))
                                {
                                    p_contract->task(p_key);
                                }
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
            p_contract->status.store(PSOBlobCompilationStatus::Started);
            p_contract->task(p_key);
        }
        for (auto& [p_key, p_contract] : m_unresolved_compute)
        {
            p_contract->status.store(PSOBlobCompilationStatus::Started);
            p_contract->task(p_key);
        }
    }
}

std::pair<PipelineState const*, PSOBlobCompilationStatus> PSOBlobCache::getGraphicsPipelineState(GraphicsPSOHandle handle) const
{
    std::unique_lock l { m_lock };
    auto it = m_cached_graphics.find(handle);
    if (it == m_cached_graphics.end())
        return std::make_pair(nullptr, PSOBlobCompilationStatus::NotScheduled);
    if (it->second.pso)
        return std::make_pair(it->second.pso.get(), PSOBlobCompilationStatus::Completed);
    GraphicsContract& contract = *it->second.p_contract;
    PSOBlobCompilationStatus status = contract.status.load();
    if (status == PSOBlobCompilationStatus::NotStarted)
    {
        PSOBlobCompilationStatus expected = PSOBlobCompilationStatus::NotStarted;
        if (contract.status.compare_exchange_strong(expected, PSOBlobCompilationStatus::Started))
        {
            contract.task(handle);
            status = contract.status.load();
        }
        else
        {
            status = expected;
        }
    }
    if (status == PSOBlobCompilationStatus::Started)
    {
        GlobalSettings const& global_settings = *m_globals.get<GlobalSettings>();
        uint32_t timeout = global_settings.getMaxNonBlockingUploadBufferAllocationTimeout();
        if (it->second.future.wait_for(std::chrono::milliseconds{ timeout }) != std::future_status::ready)
        {
            return std::make_pair(nullptr, PSOBlobCompilationStatus::Started);
        }
        status = contract.status.load();
    }
    if (status == PSOBlobCompilationStatus::Failed)
    {
        return std::make_pair(nullptr, PSOBlobCompilationStatus::Failed);
    }
    it->second.pso = it->second.future.get();
    return std::make_pair(it->second.pso.get(), contract.status.load());
}

std::pair<PipelineState const*, PSOBlobCompilationStatus> PSOBlobCache::getComputePipelineState(ComputePSOHandle handle) const
{
    std::unique_lock l { m_lock };
    auto it = m_cached_compute.find(handle);
    if (it == m_cached_compute.end())
        return std::make_pair(nullptr, PSOBlobCompilationStatus::NotScheduled);
    if (it->second.pso)
        return std::make_pair(it->second.pso.get(), PSOBlobCompilationStatus::Completed);
    ComputeContract& contract = *it->second.p_contract;
    PSOBlobCompilationStatus status = contract.status.load();
    if (status == PSOBlobCompilationStatus::NotStarted)
    {
        PSOBlobCompilationStatus expected = PSOBlobCompilationStatus::NotStarted;
        if (contract.status.compare_exchange_strong(expected, PSOBlobCompilationStatus::Started))
        {
            contract.task(handle);
            status = contract.status.load();
        }
        else
        {
            status = expected;
        }
    }
    if (status == PSOBlobCompilationStatus::Started)
    {
        GlobalSettings const& global_settings = *m_globals.get<GlobalSettings>();
        uint32_t timeout = global_settings.getMaxNonBlockingUploadBufferAllocationTimeout();
        if (it->second.future.wait_for(std::chrono::milliseconds{ timeout }) != std::future_status::ready)
        {
            return std::make_pair(nullptr, PSOBlobCompilationStatus::Started);
        }
        status = contract.status.load();
    }
    if (status == PSOBlobCompilationStatus::Failed)
    {
        return std::make_pair(nullptr, PSOBlobCompilationStatus::Failed);
    }
    it->second.pso = it->second.future.get();
    return std::make_pair(it->second.pso.get(), contract.status.load());
}

std::unique_ptr<PipelineState> PSOBlobCache::compileGraphicsPSOBlob(GraphicsPSOHandle handle)
{
    assert(handle.p_internal);

    auto cit = m_graphics_contracts.find(*handle.p_internal);
    assert(cit != m_graphics_contracts.end());

    try
    {
        GraphicsContract& contract = cit->second;

        // Retry the root-signature resolution a few times: when the RS contract is still
        // compiling, getNativeRootSignature returns (nullptr, Started) after a wait_for()
        // timeout. Each retry gives it another window before we give up.
        Microsoft::WRL::ComPtr<ID3D12RootSignature> native_rs;
        for (int attempt = 0; attempt < c_max_rs_resolution_retries; ++attempt)
        {
            auto [rs, rs_status] = m_rs_blob_cache.getNativeRootSignature(contract.rs_handle);
            if (rs && rs_status == RootSignatureBlobCompilationStatus::Completed)
            {
                native_rs = rs->native();
                break;
            }
            if (rs_status == RootSignatureBlobCompilationStatus::NotScheduled
             || rs_status == RootSignatureBlobCompilationStatus::Failed)
            {
                cit->second.status.store(PSOBlobCompilationStatus::Failed);
                return nullptr;
            }
            // NotStarted (someone else claimed it) or Started (still compiling). Loop and
            // let getNativeRootSignature do another wait_for() pass.
        }
        if (!native_rs)
        {
            cit->second.status.store(PSOBlobCompilationStatus::Failed);
            return nullptr;
        }

        D3DDataBlob precached_pso_blob = loadPrecachedPSOBlob(m_gpu_blob_cache, *handle.p_internal);

        std::unique_ptr<PipelineState> pso{};

        if (precached_pso_blob)
        {
            try
            {
                pso = std::make_unique<PipelineState>(m_globals, native_rs, contract.descriptor, precached_pso_blob);
            }
            catch (Exception const&)
            {
                misc::Log::retrieve()->out("Unable to compile graphics PSO using pre-cached blob. The cache might be corrupted", misc::LogMessageType::exclamation);
                precached_pso_blob = nullptr;
            }
        }

        if (!pso)
        {
            // failed to create pso using pre-cached blob, or pre-cached blob wasn't available
            pso = std::make_unique<PipelineState>(m_globals, native_rs, contract.descriptor, nullptr);
        }

        if (!precached_pso_blob && m_gpu_blob_cache)
        {
            m_gpu_blob_cache.put(*handle.p_internal, makeSharedDataChunk(pso->getCache()));
        }

        cit->second.status.store(PSOBlobCompilationStatus::Completed);
        return pso;
    }
    catch (Exception const& e)
    {
        LEXGINE_LOG_ERROR(this, std::string { "Failed to compile graphics PSO: " } + e.what());
    }
    cit->second.status.store(PSOBlobCompilationStatus::Failed);
    return nullptr;
}

std::unique_ptr<PipelineState> PSOBlobCache::compileComputePSOBlob(ComputePSOHandle handle)
{
    assert(handle.p_internal);

    auto cit = m_compute_contracts.find(*handle.p_internal);
    assert(cit != m_compute_contracts.end());

    try
    {
        ComputeContract& contract = cit->second;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> native_rs;
        for (int attempt = 0; attempt < c_max_rs_resolution_retries; ++attempt)
        {
            auto [rs, rs_status] = m_rs_blob_cache.getNativeRootSignature(contract.rs_handle);
            if (rs && rs_status == RootSignatureBlobCompilationStatus::Completed)
            {
                native_rs = rs->native();
                break;
            }
            if (rs_status == RootSignatureBlobCompilationStatus::NotScheduled
             || rs_status == RootSignatureBlobCompilationStatus::Failed)
            {
                cit->second.status.store(PSOBlobCompilationStatus::Failed);
                return nullptr;
            }
        }
        if (!native_rs)
        {
            cit->second.status.store(PSOBlobCompilationStatus::Failed);
            return nullptr;
        }

        D3DDataBlob precached_pso_blob = loadPrecachedPSOBlob(m_gpu_blob_cache, *handle.p_internal);

        std::unique_ptr<PipelineState> pso{};

        if (precached_pso_blob)
        {
            try
            {
                pso = std::make_unique<PipelineState>(m_globals, native_rs, contract.descriptor, precached_pso_blob);
            }
            catch (Exception const&)
            {
                misc::Log::retrieve()->out("Unable to compile compute PSO using pre-cached blob. The cache might be corrupted", misc::LogMessageType::exclamation);
                precached_pso_blob = nullptr;
            }
        }

        if (!pso)
        {
            // failed to create pso using pre-cached blob, or pre-cached blob wasn't available
            pso = std::make_unique<PipelineState>(m_globals, native_rs, contract.descriptor, nullptr);
        }

        if (!precached_pso_blob && m_gpu_blob_cache)
        {
            m_gpu_blob_cache.put(*handle.p_internal, makeSharedDataChunk(pso->getCache()));
        }

        cit->second.status.store(PSOBlobCompilationStatus::Completed);
        return pso;
    }
    catch (Exception const& e)
    {
        LEXGINE_LOG_ERROR(this, std::string { "Failed to compile compute PSO: " } + e.what());
    }
    cit->second.status.store(PSOBlobCompilationStatus::Failed);
    return nullptr;
}

GpuDataBlobCacheKey PSOBlobCache::createGraphicsGpuDataBlobCacheKey(
    GraphicsPSODescriptor const& descriptor,
    RootSignatureHandle rs_handle) const
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
    RootSignatureHandle rs_handle) const
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
