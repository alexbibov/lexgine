#include "ranges"

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

GpuDataBlobCacheKey const* RootSignatureBlobCache::createRootSignatureBlobCompilationContract(
    RootSignature&& root_signature,
    RootSignatureFlags const& flags
)
{
    GpuDataBlobCacheKey key = createGpuDataBlobCacheKey(*root_signature.hash(), flags);
    auto cit = m_contracts.find(key);
    if (cit != m_contracts.end())
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(
            this, 
            std::format("Attempt to create duplicate root signature with key {}", key.toString())
        );
    }
    auto [ncit, res] = m_contracts.emplace(
        std::make_pair(
            std::move(key),
            RootSignatureDeferredBlobCompilationContract {
                .root_signature = std::move(root_signature),
                .flags = flags,
                .task = std::packaged_task<Microsoft::WRL::ComPtr<ID3D12RootSignature>(GpuDataBlobCacheKey const*)> { std::bind(&RootSignatureBlobCache::compileRootSignatureBlob, this, std::placeholders::_1) },
                .is_ready = false
            }
        )
    );
    m_futures_vector_lut[ncit->first] = m_futures.size();
    m_all_keys.push_back(&ncit->first);
    m_futures.push_back(ncit->second.task.get_future());
    return m_all_keys.back();
}

void RootSignatureBlobCache::createRootSignatures()
{
    if (m_contracts.empty())
        return;

    if (m_async_rs_creation)
    {
        size_t num_threads = m_globals.get<GlobalSettings>()->getNumberOfWorkers();
        size_t per_bucket_count = m_contracts.size() / num_threads;
        size_t rem = m_contracts.size() % num_threads;
        for (size_t i = 0; i < num_threads; ++i) 
        {
            std::thread t 
            {
                [this](size_t start, size_t count) 
                {
                    for (size_t j = start; j < start + count; ++j) 
                    {
                        const GpuDataBlobCacheKey* p_key = m_all_keys[j];
                        RootSignatureDeferredBlobCompilationContract& contract = m_contracts[*p_key];
                        contract.task(p_key);
                        contract.is_ready.store(true);
                    }
                },
                per_bucket_count * i + (i < rem ? i : rem),
                per_bucket_count + (i < rem ? 1 : 0)
            };
            t.detach();
        }
    }
    else
    {
        for (auto& [k, c] : m_contracts)
        {
            c.task(&k);
            c.is_ready.store(true);
        }
    }
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureBlobCache::getNativeRootSignature(GpuDataBlobCacheKey const* key) const
{
    auto it = m_contracts.find(*key);
    if (it == m_contracts.end() || !it->second.is_ready.load())
    {
        return nullptr;
    }
    return m_futures[m_futures_vector_lut.at(*key)].get();
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureBlobCache::compileRootSignatureBlob(
    GpuDataBlobCacheKey const* key)
{
    D3DDataBlob rs_blob { nullptr };
    SharedDataChunk cached_rs_blob {};
    if (m_gpu_blob_cache) {
        cached_rs_blob = m_gpu_blob_cache.find(*key);
    }
    if (cached_rs_blob.size() && cached_rs_blob.data()) {
        Microsoft::WRL::ComPtr<ID3DBlob> d3d_blob { nullptr };
        HRESULT hres = D3DCreateBlob(cached_rs_blob.size(), d3d_blob.GetAddressOf());
        if (hres == S_OK || hres == S_FALSE) {
            memcpy(d3d_blob->GetBufferPointer(), cached_rs_blob.data(), cached_rs_blob.size());
            rs_blob = D3DDataBlob { d3d_blob };
        }
    }
    if (!rs_blob) {
        auto cit = m_contracts.find(*key);
        if (cit == m_contracts.end()) {
            return nullptr;
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
            m_gpu_blob_cache.put(*key, cached_rs_blob);
        }
    }
    assert(rs_blob);
    return m_device.createRootSignature(rs_blob);
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

bool RootSignatureBlobCache::isReady(GpuDataBlobCacheKey const* key) const
{
    return m_contracts.at(*key).is_ready.load();
}
bool RootSignatureBlobCache::isReady() const
{
    for (auto& c : std::views::values(m_contracts))
    {
        if (!c.is_ready.load())
            return false;
    }
    return true;
}

bool RootSignatureBlobCache::waitTillReady(const std::chrono::milliseconds& timeout) const
{
    for (auto& [k, future_offset] : m_futures_vector_lut)
    {
        m_futures[future_offset].wait_for(timeout);
        if (!m_contracts.at(k).is_ready.load())
            return false;
    }
    return true;
}


}
