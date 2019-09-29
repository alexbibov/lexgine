#ifndef LEXGINE_CORE_DX_D3D12_QUERY_CACHE_H
#define LEXGINE_CORE_DX_D3D12_QUERY_CACHE_H

#include <cstdint>
#include <array>
#include <mutex>

#include <wrl.h>
#include <d3d12.h>

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/misc/optional.h"



namespace lexgine::core::dx::d3d12 {

struct StreamOutputStatistics
{
    uint64_t primitives_written_count;
    uint64_t primitives_storage_needed;
};

struct PipelineStatistics final
{
    uint64_t vertex_count;
    uint64_t primitive_count;
    uint64_t vs_invocation_count;
    uint64_t gs_invocation_count;
    uint64_t gs_output_primitive_count;
    uint64_t rasterizer_primitive_count;
    uint64_t drawn_primitive_count;
    uint64_t ps_invocation_count;
    uint64_t hs_invocation_count;
    uint64_t ds_invocation_count;
    uint64_t cs_invocation_count;
};

struct QueryHandle final
{
    uint8_t heap_id;
    uint32_t query_id;
    uint32_t query_count;
    D3D12_QUERY_TYPE query_type;
    uint32_t mutable offset;
};

class QueryData final
{
    friend class QueryCache;

public:
    QueryData(QueryData const&) = delete;

    QueryData(QueryData&& other) noexcept;

    ~QueryData();

    template<typename T>
    void get(T& value, size_t index)
    {
        value = *(static_cast<T const*>(m_mapped_addr) + index);
    }

private:
    QueryData(CommittedResource const& query_resolve_buffer,
        size_t data_offset, size_t data_range, std::atomic_bool& checker);

private:
    CommittedResource const& m_query_resolve_buffer;
    std::atomic_bool& m_checker;
    void const* m_mapped_addr;
};

template<typename T> class QueryCacheAttorney;

class QueryCache final : public NamedEntity<class_names::D3D12_QueryCache>
{
    friend class QueryCacheAttorney<CommandList>;
public:
    QueryCache(GlobalSettings const& settings, Device& device);

    //! Registers occlusion query
    QueryHandle registerOcclusionQuery(bool is_binary_occlusion, uint32_t query_count = 1);

    //! Registers new timestamp query, which can be used on direct and compute command queues
    QueryHandle registerTimestampQuery(uint32_t query_count = 1);

    //! Registers new timestamp query, which can be used on copy command queues
    QueryHandle registerCopyQueueTimestampQuery(uint32_t query_count = 1);

    //! Registers new pipeline statistics query
    QueryHandle registerPipelineStatisticsQuery(uint32_t query_count = 1);

    //! Registers new stream output query
    QueryHandle registerStreamOutputQuery(uint8_t stream_output_id, uint32_t query_count = 1);

    QueryData fetchQuery(QueryHandle const& query_handle) const;

    //! Initializes query cache
    void initQueryCache();

    //! Write query flush commands into supplied command list
    void writeFlushCommandList(CommandList const& cmd_list) const;

    bool isResolveBufferStillMapped() const { return m_resolve_buffer_mapped.load(std::memory_order_acquire); }

private:
    static uint8_t constexpr c_query_heap_count = 5;
    static uint8_t constexpr c_query_type_count = 9;

private:
    inline size_t getHeapQueryCount(uint8_t heap_id) const;
    inline size_t getHeapSize(uint8_t heap_id) const;
    inline size_t getHeapSegmentOffset(uint8_t heap_id) const;
    
private:
    GlobalSettings const& m_settings;    //! global engine settings
    Device& m_device;    //! device, for which to create the query heap cache
    FrameProgressTracker const& m_frame_progress_tracker;
    std::array<uint32_t, c_query_type_count> m_query_heap_capacities;    //!< capacities of the query heaps
    std::array<Microsoft::WRL::ComPtr<ID3D12QueryHeap>, c_query_heap_count> m_query_heaps;    //!< cached query heaps
    std::unique_ptr<CommittedResource> m_query_resolve_buffer;    //! resolution buffer employed by the queries
    size_t m_per_frame_resolve_buffer_capacity;    //! resolve buffer capacity reserved for each frame

    mutable std::atomic_bool m_resolve_buffer_mapped{ false };
};

template<> class QueryCacheAttorney<CommandList>
{
    friend class CommandList;

    static ID3D12QueryHeap* getNativeQueryHeap(QueryCache const& parent_query_cache, uint32_t heap_id)
    {
        return parent_query_cache.m_query_heaps[heap_id].Get();
    }
};

}

#endif
