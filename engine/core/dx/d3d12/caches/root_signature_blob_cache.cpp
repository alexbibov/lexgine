#include "utility"
#include "tuple"

#include "d3dcompiler.h"

#include "engine/core/globals.h"
#include "engine/core/global_settings.h"
#include "engine/core/exception.h"
#include "engine/core/gpu_data_blob_cache_key.h"
#include "engine/core/gpu_data_blob_cache.h"
#include "engine/core/dx/dxgi/hw_adapter_enumerator.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/root_signature.h"
#include "engine/core/dx/d3d12/d3d_data_blob.h"

#include "root_signature_blob_cache.h"

namespace lexgine::core::dx::d3d12::caches {

RootSignatureBlobCache::RootSignatureBlobCache(Globals& globals)
    : m_globals { globals }
    , m_device { *globals.get<Device>() }
    , m_gpu_blob_cache { *globals.get<GpuDataBlobCache>() }
    , m_async_rs_creation { globals.get<GlobalSettings>()->isDeferredGpuResourceCompilationOn() }
{
}

RootSignatureBlobCache::~RootSignatureBlobCache()
{
    waitTillReady();
}

RootSignatureHandle RootSignatureBlobCache::createRootSignatureBlobCompilationContract(
    RootSignature&& root_signature,
    RootSignatureFlags const& flags
)
{
    std::unique_lock l { m_lock };
    waitTillReady();
    GpuDataBlobCacheKey key = createGpuDataBlobCacheKey(*root_signature.hash(), flags);
    auto cit = m_contracts.find(key);
    if (cit != m_contracts.end())
    {
        return { &cit->first };
    }
    auto [ncit, res] = m_contracts.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::move(key)),
        std::forward_as_tuple(std::move(root_signature), flags, std::bind(&RootSignatureBlobCache::compileRootSignatureBlob, this, std::placeholders::_1))
    );
    m_cached_rs.emplace(
        std::make_pair(
            RootSignatureHandle { .p_internal = &ncit->first },
            RootSignatureCompilationResult {
                .p_contract = &ncit->second,
                .future = ncit->second.task.get_future(),
                .rs = nullptr }));
    return { &ncit->first };
}

void RootSignatureBlobCache::createRootSignatures()
{
    std::unique_lock l { m_lock };
    waitTillReady();
    m_unresolved_contracts.clear();
    m_unresolved_contracts.reserve(m_contracts.size());
    for (auto& [k, c] : m_contracts)
    {
        if (c.status.load() == RootSignatureBlobCompilationStatus::NotStarted)
        {
            m_unresolved_contracts.push_back(std::make_pair(RootSignatureHandle { &k }, &c));
        }
    }
    if (m_unresolved_contracts.empty())
        return;
    size_t num_threads = m_globals.get<GlobalSettings>()->getNumberOfWorkers();
    if (m_async_rs_creation && num_threads > 0)
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
                std::thread
                {
                    [this](size_t start, size_t count) 
                    {
                        for (size_t j = start; j < start + count; ++j) 
                        {
                            auto [p_key, p_contract] = m_unresolved_contracts[j];
                            p_contract->task(p_key);
                        }
                    },
                    per_bucket_count * i + (i < rem ? i : rem),
                    per_bucket_count + (i < rem ? 1 : 0)
                }
            );
        }
    }
    else
    {
        for (auto& [p_key, p_contract] : m_unresolved_contracts)
        {
            p_contract->task(p_key);
        }
    }
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureBlobCache::getNativeRootSignature(RootSignatureHandle key) const
{
    std::unique_lock l { m_lock };
    auto rsit = m_cached_rs.find(key);
    if (rsit == m_cached_rs.end())
        return nullptr;
    if (rsit->second.rs)
        return rsit->second.rs;
    RootSignatureDeferredBlobCompilationContract const& contract = *rsit->second.p_contract;
    if (contract.status.load() != RootSignatureBlobCompilationStatus::Completed)
        return nullptr;
    rsit->second.rs = rsit->second.future.get();
    assert(rsit->second.rs);
    return rsit->second.rs;
 }

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureBlobCache::compileRootSignatureBlob(
     RootSignatureHandle key)
 {
    assert(key.p_internal);

    auto cit = m_contracts.find(*key.p_internal);
    assert(cit != m_contracts.end());

    cit->second.status.store(RootSignatureBlobCompilationStatus::Failed);

    D3DDataBlob rs_blob { nullptr };
    SharedDataChunk cached_rs_blob {};
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rs { nullptr };

    if (m_gpu_blob_cache) {
        cached_rs_blob = m_gpu_blob_cache.find(*key.p_internal);
    }

    if (cached_rs_blob.size() && cached_rs_blob.data()) {
        Microsoft::WRL::ComPtr<ID3DBlob> d3d_blob { nullptr };
        HRESULT hres = D3DCreateBlob(cached_rs_blob.size(), d3d_blob.GetAddressOf());
        if (SUCCEEDED(hres)) {
            memcpy(d3d_blob->GetBufferPointer(), cached_rs_blob.data(), cached_rs_blob.size());
            rs_blob = D3DDataBlob { d3d_blob };
        }
        if (rs_blob) 
        {
            rs = m_device.createRootSignature(rs_blob);
            if (rs && !m_device.getErrorState()) 
            {
                cit->second.status.store(RootSignatureBlobCompilationStatus::Completed);
                return rs;
            }
        }
    }
    
    RootSignatureDeferredBlobCompilationContract& contract = cit->second;
    rs_blob = contract.root_signature.compile(contract.flags);
    if (contract.root_signature.getErrorState())
    {
        return nullptr;
    }
    if (m_gpu_blob_cache) {
        cached_rs_blob = SharedDataChunk { rs_blob.size() };
        memcpy(cached_rs_blob.data(), rs_blob.data(), rs_blob.size());
        m_gpu_blob_cache.put(*key.p_internal, cached_rs_blob);
    }
    rs = m_device.createRootSignature(rs_blob);
    if (rs && !m_device.getErrorState()) 
    {
        cit->second.status.store(RootSignatureBlobCompilationStatus::Completed);
        return rs;
    }

    return nullptr;
}

GpuDataBlobCacheKey RootSignatureBlobCache::createGpuDataBlobCacheKey(
    misc::HashValue const& hashValue,
	RootSignatureFlags const& flags
)
{
    LUID adapter_luid = m_device.hwAdapter()->getProperties().details.luid;
    misc::UUID gpu_driver_uuid { 
        static_cast<uint64_t>(adapter_luid.LowPart), 
        static_cast<uint64_t>(adapter_luid.HighPart) 
    };
    GpuDataBlobCacheKey::CommonManifest manifest = GpuDataBlobCacheKey::createManifest(
        { 'R', 'S', 'I', 'G' },
        gpu_driver_uuid,
        hashValue
    );
    InternalKey rs_key { 
        .root_signature_version = RootSignature::c_root_signature_version,
        .root_signature_compilation_flags = static_cast<uint32_t>(flags.getValue())
    };
    return GpuDataBlobCacheKey { manifest, rs_key };
}

void RootSignatureBlobCache::waitTillReady()
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
