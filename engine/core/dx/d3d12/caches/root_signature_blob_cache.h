#ifndef LEXGINE_CORE_DX_D3D12_CACHES_ROOT_SIGNATURE_BLOB_CACHE_H
#define LEXGINE_CORE_DX_D3D12_CACHES_ROOT_SIGNATURE_BLOB_CACHE_H

#include <unordered_map>
#include <future>
#include <atomic>
#include <functional>
#include <vector>
#include <mutex>
#include <thread>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/class_names.h"
#include "engine/core/gpu_data_blob_cache_key.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/root_signature.h"

namespace lexgine::core::dx::d3d12::caches {

enum class RootSignatureBlobCompilationStatus 
{
    NotScheduled,
    NotStarted,
    Started,
    Completed,
    Failed
};

struct RootSignatureHandle
{
    GpuDataBlobCacheKey const* p_internal;
    bool operator==(RootSignatureHandle const& other) const
    {
        return p_internal == other.p_internal;
    }
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
    RootSignatureHandle createRootSignatureBlobCompilationContract(
        RootSignature&& root_signature,
        RootSignatureFlags const& flags
    );
    void createRootSignatures();
    void waitTillReady();
    std::pair<Microsoft::WRL::ComPtr<ID3D12RootSignature>, RootSignatureBlobCompilationStatus> getNativeRootSignature(RootSignatureHandle key) const;

private:
    struct RootSignatureDeferredBlobCompilationContract
    {
        RootSignature root_signature;
        RootSignatureFlags flags;
        std::packaged_task<Microsoft::WRL::ComPtr<ID3D12RootSignature>(RootSignatureHandle)> task;
        std::atomic<RootSignatureBlobCompilationStatus> status;

        RootSignatureDeferredBlobCompilationContract(
            RootSignature&& rs, 
            RootSignatureFlags const& rs_flags,
            std::function<Microsoft::WRL::ComPtr<ID3D12RootSignature>(RootSignatureHandle)> const& op
        )
            : root_signature{ std::move(rs) }
            , flags{ rs_flags }
            , task { op }
            , status { RootSignatureBlobCompilationStatus::NotStarted }
        {

        }
    };

    struct RootSignatureCompilationResult
    {
        RootSignatureDeferredBlobCompilationContract* p_contract;
        std::future<Microsoft::WRL::ComPtr<ID3D12RootSignature>> future;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rs;
    };

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> compileRootSignatureBlob(
        RootSignatureHandle key);
    GpuDataBlobCacheKey createGpuDataBlobCacheKey(
        misc::HashValue const& hashValue,
        RootSignatureFlags const& flags
    ) const;

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
    std::vector<std::pair<RootSignatureHandle, RootSignatureDeferredBlobCompilationContract*>> m_unresolved_contracts;
    mutable std::unordered_map<
        RootSignatureHandle, 
        RootSignatureCompilationResult,
        LightWeightKeyHasher<RootSignatureHandle>
    > m_cached_rs;
    std::vector<std::thread> m_worker_threads;
};

}

#endif
