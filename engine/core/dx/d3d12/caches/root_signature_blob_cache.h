#ifndef LEXGINE_CORE_DX_D3D12_CACHES_ROOT_SIGNATURE_BLOB_CACHE_H
#define LEXGINE_CORE_DX_D3D12_CACHES_ROOT_SIGNATURE_BLOB_CACHE_H

#include "unordered_map"
#include "future"
#include "atomic"
#include "functional"
#include "vector"
#include "mutex"
#include "thread"

#include "engine/core/entity.h"
#include "engine/core/class_names.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/root_signature.h"


namespace lexgine::core::dx::d3d12::caches {

enum class RootSignatureBlobCompilationStatus 
{
    NotStarted,
    Completed,
    Failed
};

class RootSignatureBlobCache final : public NamedEntity<class_names::D3D12_RootSignatureBlobCache>
{
public:
    struct InternalKey
    {
        uint32_t root_signature_version;
        uint32_t root_signature_compilation_flags;
    };

public:
    RootSignatureBlobCache(Globals& globals);
    ~RootSignatureBlobCache();
    GpuDataBlobCacheKey const* createRootSignatureBlobCompilationContract(
        RootSignature&& root_signature,
        RootSignatureFlags const& flags
    );
    void createRootSignatures();
    void waitTillReady();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> getNativeRootSignature(GpuDataBlobCacheKey const* key) const;

private:
    struct RootSignatureDeferredBlobCompilationContract
    {
        RootSignature root_signature;
        RootSignatureFlags flags;
        std::packaged_task<Microsoft::WRL::ComPtr<ID3D12RootSignature>(GpuDataBlobCacheKey const*)> task;
        std::atomic<RootSignatureBlobCompilationStatus> status;
        RootSignatureDeferredBlobCompilationContract(
            RootSignature&& rs, 
            RootSignatureFlags const& rs_flags,
            std::function<Microsoft::WRL::ComPtr<ID3D12RootSignature>(GpuDataBlobCacheKey const*)> const& task
        )
            : root_signature{ std::move(rs) }
            , flags{ rs_flags }
            , task { task }
            , status { RootSignatureBlobCompilationStatus::NotStarted }
        {

        }
    };

    struct RootSignatureCompilationResult
    {
        std::future<Microsoft::WRL::ComPtr<ID3D12RootSignature>> future;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rs;
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
    mutable std::recursive_mutex m_lock;
    std::unordered_map<
        GpuDataBlobCacheKey,
        RootSignatureDeferredBlobCompilationContract,
        GpuDataBlobCacheKeyHasher
    > m_contracts;
    std::vector<std::pair<GpuDataBlobCacheKey const*, RootSignatureDeferredBlobCompilationContract*>> m_unresolved_contracts;
    mutable std::unordered_map<
        GpuDataBlobCacheKey const*, 
        RootSignatureCompilationResult
    > m_cached_rs;
    std::vector<std::thread> m_worker_threads;
};

}

#endif
