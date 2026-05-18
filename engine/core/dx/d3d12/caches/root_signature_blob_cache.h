#ifndef LEXGINE_CORE_DX_D3D12_CACHES_ROOT_SIGNATURE_BLOB_CACHE_H
#define LEXGINE_CORE_DX_D3D12_CACHES_ROOT_SIGNATURE_BLOB_CACHE_H
#include "unordered_map"
#include "future"
#include "chrono"

#include "engine/core/entity.h"
#include "engine/core/class_names.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/root_signature.h"


namespace lexgine::core::dx::d3d12::caches {

class RootSignatureBlobCache : public NamedEntity<class_names::D3D12_RootSignatureBlobCache>
{
public:
    struct InternalKey
    {
        uint32_t root_signature_version;
        uint32_t root_signature_compilation_flags;
    };

public:
    RootSignatureBlobCache(Globals& globals);
    GpuDataBlobCacheKey const* createRootSignatureBlobCompilationContract(
        RootSignature&& root_signature,
        RootSignatureFlags const& flags
    );
    void createRootSignatures();
    bool isReady(GpuDataBlobCacheKey const* key) const;
    bool isReady() const;
    bool waitTillReady(const std::chrono::milliseconds& timeout) const;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> getNativeRootSignature(GpuDataBlobCacheKey const* key) const;

private:
    struct RootSignatureDeferredBlobCompilationContract
    {
        RootSignature root_signature;
        RootSignatureFlags flags;
        std::packaged_task<Microsoft::WRL::ComPtr<ID3D12RootSignature>(GpuDataBlobCacheKey const*)> task;
        std::atomic<bool> is_ready { false };
    };

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> compileRootSignatureBlob(
        GpuDataBlobCacheKey const* key);
    GpuDataBlobCacheKey createGpuDataBlobCacheKey(
        misc::HashValue const& hashValue,
        RootSignatureFlags const& flags
    );

private:
    Globals& m_globals;
    Device& m_device;
    GpuDataBlobCache& m_gpu_blob_cache;
    bool m_async_rs_creation;
    std::unordered_map<
        GpuDataBlobCacheKey,
        RootSignatureDeferredBlobCompilationContract,
        GpuDataBlobCacheKeyHasher
    > m_contracts;
    std::unordered_map<GpuDataBlobCacheKey, size_t> m_futures_vector_lut;
    std::vector<GpuDataBlobCacheKey const*> m_all_keys;
    mutable std::vector<std::future<Microsoft::WRL::ComPtr<ID3D12RootSignature>>> m_futures;
};

}

#endif
