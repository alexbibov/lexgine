#ifndef LEXGINE_CORE_DX_D3D12_CACHES_PSO_BLOB_CACHE_H
#define LEXGINE_CORE_DX_D3D12_CACHES_PSO_BLOB_CACHE_H

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/gpu_data_blob_cache_key.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/caches/lexgine_core_dx_d3d12_caches_fwd.h"
#include "engine/core/dx/d3d12/caches/root_signature_blob_cache.h"
#include "engine/core/dx/d3d12/pipeline_state.h"

namespace lexgine::core::dx::d3d12::caches {

enum class PSOBlobCompilationStatus
{
    NotScheduled,
    NotStarted,
    Started,
    Completed,
    Failed
};

struct GraphicsPSOHandle
{
    GpuDataBlobCacheKey const* p_internal;
    bool operator==(GraphicsPSOHandle const& other) const { return p_internal == other.p_internal; }
};

struct ComputePSOHandle
{
    GpuDataBlobCacheKey const* p_internal;
    bool operator==(ComputePSOHandle const& other) const { return p_internal == other.p_internal; }
};

class PSOBlobCache final : public NamedEntity<PSOBlobCache>
{
public:
    struct InternalKey
    {
        uint32_t pso_format_version;
    };

    static constexpr uint32_t c_pso_format_version = 1;

public:
    PSOBlobCache(Globals& globals);
    ~PSOBlobCache();

    GraphicsPSOHandle createGraphicsPSOBlobCompilationContract(
        GraphicsPSODescriptor const& descriptor,
        RootSignatureHandle root_signature_handle);

    ComputePSOHandle createComputePSOBlobCompilationContract(
        ComputePSODescriptor const& descriptor,
        RootSignatureHandle root_signature_handle);

    void createPipelineStates();
    void waitTillReady();

    std::pair<PipelineState const*, PSOBlobCompilationStatus> getGraphicsPipelineState(GraphicsPSOHandle handle) const;
    std::pair<PipelineState const*, PSOBlobCompilationStatus> getComputePipelineState(ComputePSOHandle handle) const;

private:
    static constexpr int c_max_rs_resolution_retries = 3;

    struct GraphicsContract
    {
        GraphicsPSODescriptor descriptor;
        RootSignatureHandle rs_handle;
        std::packaged_task<std::unique_ptr<PipelineState>(GraphicsPSOHandle)> task;
        std::atomic<PSOBlobCompilationStatus> status;

        GraphicsContract(
            GraphicsPSODescriptor const& descriptor,
            RootSignatureHandle rs_handle,
            std::function<std::unique_ptr<PipelineState>(GraphicsPSOHandle)> const& op)
            : descriptor { descriptor }
            , rs_handle { rs_handle }
            , task { op }
            , status { PSOBlobCompilationStatus::NotStarted }
        {
        }
    };

    struct ComputeContract
    {
        ComputePSODescriptor descriptor;
        RootSignatureHandle rs_handle;
        std::packaged_task<std::unique_ptr<PipelineState>(ComputePSOHandle)> task;
        std::atomic<PSOBlobCompilationStatus> status;

        ComputeContract(
            ComputePSODescriptor const& descriptor,
            RootSignatureHandle rs_handle,
            std::function<std::unique_ptr<PipelineState>(ComputePSOHandle)> const& op)
            : descriptor { descriptor }
            , rs_handle { rs_handle }
            , task { op }
            , status { PSOBlobCompilationStatus::NotStarted }
        {
        }
    };

    struct GraphicsResult
    {
        GraphicsContract* p_contract;
        std::future<std::unique_ptr<PipelineState>> future;
        std::unique_ptr<PipelineState> pso;
    };

    struct ComputeResult
    {
        ComputeContract* p_contract;
        std::future<std::unique_ptr<PipelineState>> future;
        std::unique_ptr<PipelineState> pso;
    };

private:
    std::unique_ptr<PipelineState> compileGraphicsPSOBlob(GraphicsPSOHandle handle);
    std::unique_ptr<PipelineState> compileComputePSOBlob(ComputePSOHandle handle);

    GpuDataBlobCacheKey createGraphicsGpuDataBlobCacheKey(
        GraphicsPSODescriptor const& descriptor,
        RootSignatureHandle rs_handle) const;
    GpuDataBlobCacheKey createComputeGpuDataBlobCacheKey(
        ComputePSODescriptor const& descriptor,
        RootSignatureHandle rs_handle) const;

private:
    Globals& m_globals;
    Device& m_device;
    GpuDataBlobCache& m_gpu_blob_cache;
    RootSignatureBlobCache& m_rs_blob_cache;
    bool m_async_pso_creation;
    mutable std::recursive_mutex m_lock;

    std::unordered_map<
        GpuDataBlobCacheKey,
        GraphicsContract,
        GpuDataBlobCacheKeyHasher
    > m_graphics_contracts;

    std::unordered_map<
        GpuDataBlobCacheKey,
        ComputeContract,
        GpuDataBlobCacheKeyHasher
    > m_compute_contracts;

    std::vector<std::pair<GraphicsPSOHandle, GraphicsContract*>> m_unresolved_graphics;
    std::vector<std::pair<ComputePSOHandle, ComputeContract*>> m_unresolved_compute;

    mutable std::unordered_map<
        GraphicsPSOHandle,
        GraphicsResult,
        LightWeightKeyHasher<GraphicsPSOHandle>
    > m_cached_graphics;

    mutable std::unordered_map<
        ComputePSOHandle,
        ComputeResult,
        LightWeightKeyHasher<ComputePSOHandle>
    > m_cached_compute;

    std::vector<std::thread> m_worker_threads;
};

}

#endif
