#include <vector>
#include <numeric>

#include "lexgine/core/exception.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/misc/misc.h"
#include "device.h"
#include "command_list.h"
#include "resource.h"

#include "query_cache.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

namespace {

enum class QueryHeapType : uint8_t
{
    occlusion,
    timestamp,
    copy_queue_timestamp,
    pipeline_statistics,
    so_statistics,
    count
};

std::array<std::vector<uint8_t>, static_cast<size_t>(QueryHeapType::count)> query_heap_type_to_capacity_cache_map{ {
    {0, 1}, {2}, {3}, {4}, {5, 6, 7, 8}
} };

D3D12_QUERY_HEAP_TYPE toNativeQueryHeapType(uint8_t heap_id)
{
    QueryHeapType query_heap_type = static_cast<QueryHeapType>(heap_id);
    switch (query_heap_type)
    {
    case QueryHeapType::occlusion: return D3D12_QUERY_HEAP_TYPE_OCCLUSION;
    case QueryHeapType::timestamp: return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    case QueryHeapType::copy_queue_timestamp: return D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP;
    case QueryHeapType::pipeline_statistics: return D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
    case QueryHeapType::so_statistics: return D3D12_QUERY_HEAP_TYPE_SO_STATISTICS;
    default:
        __assume(0);
    }
}

size_t getRequiredStoragePerQuery(uint8_t heap_id)
{
    QueryHeapType query_heap_type = static_cast<QueryHeapType>(heap_id);
    switch (query_heap_type)
    {
    case QueryHeapType::occlusion:
    case QueryHeapType::timestamp:
    case QueryHeapType::copy_queue_timestamp:
        return sizeof(uint64_t);
    case QueryHeapType::so_statistics:
        return sizeof(D3D12_QUERY_DATA_SO_STATISTICS);
    case QueryHeapType::pipeline_statistics:
        return sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
    default:
        __assume(0);
        break;
    }
}

}


QueryData::QueryData(CommittedResource const& query_resolve_buffer, size_t data_offset, size_t data_range, 
    std::atomic_bool& checker)
    : m_query_resolve_buffer{ query_resolve_buffer }
    , m_checker{ checker }
{
    m_mapped_addr = query_resolve_buffer.map(0, data_offset, data_range);
    m_checker.store(true, std::memory_order_release);
}

QueryData::QueryData(QueryData&& other) noexcept
    : m_query_resolve_buffer{ other.m_query_resolve_buffer }
    , m_checker{ other.m_checker }
    , m_mapped_addr{ other.m_mapped_addr }
{
    other.m_mapped_addr = nullptr;
}

QueryData::~QueryData()
{
    if (m_mapped_addr)
    {
        m_query_resolve_buffer.unmap(0, false);
        m_checker.store(false, std::memory_order_release);
    }
}


QueryCache::QueryCache(GlobalSettings const& settings, Device& device)
    : m_settings{ settings }
    , m_device{ device }
    , m_frame_progress_tracker{ device.frameProgressTracker() }
{
    static_assert(static_cast<uint8_t>(QueryHeapType::count) == c_query_heap_count);
    static_assert(c_query_heap_count + 4 == c_query_type_count);
}

QueryHandle QueryCache::registerOcclusionQuery(bool is_binary_occlusion, uint32_t query_count/* = 1*/)
{
    uint8_t const heap_id = static_cast<uint8_t>(QueryHeapType::occlusion);
    auto& current_capacity = m_query_heap_capacities[query_heap_type_to_capacity_cache_map[heap_id][is_binary_occlusion ? 1 : 0]];
    QueryHandle rv{ heap_id, current_capacity, query_count, is_binary_occlusion ? D3D12_QUERY_TYPE_BINARY_OCCLUSION : D3D12_QUERY_TYPE_OCCLUSION };
    current_capacity += query_count;
    return rv;
}

QueryHandle QueryCache::registerTimestampQuery(uint32_t query_count/* = 1*/)
{
    uint8_t const heap_id = static_cast<uint8_t>(QueryHeapType::timestamp);
    auto& current_capacity = m_query_heap_capacities[query_heap_type_to_capacity_cache_map[heap_id][0]];
    QueryHandle rv{ heap_id, current_capacity, query_count, D3D12_QUERY_TYPE_TIMESTAMP };
    current_capacity += query_count;
    return rv;
}

QueryHandle QueryCache::registerCopyQueueTimestampQuery(uint32_t query_count/* = 1*/)
{
    uint8_t const heap_id = m_settings.isAsyncCopyEnabled()
        ? static_cast<uint8_t>(QueryHeapType::copy_queue_timestamp)
        : static_cast<uint8_t>(QueryHeapType::timestamp);
    auto& current_capacity = m_query_heap_capacities[query_heap_type_to_capacity_cache_map[heap_id][0]];
    QueryHandle rv{ heap_id, current_capacity, query_count, D3D12_QUERY_TYPE_TIMESTAMP };
    current_capacity += query_count;
    return rv;
}

QueryHandle QueryCache::registerPipelineStatisticsQuery(uint32_t query_count/* = 1*/)
{
    uint8_t const heap_id = static_cast<uint8_t>(QueryHeapType::pipeline_statistics);
    auto& current_capacity = m_query_heap_capacities[query_heap_type_to_capacity_cache_map[heap_id][0]];
    QueryHandle rv{ heap_id, current_capacity, query_count, D3D12_QUERY_TYPE_PIPELINE_STATISTICS };
    current_capacity += query_count;
    return rv;
}

QueryHandle QueryCache::registerStreamOutputQuery(uint8_t stream_output_id, uint32_t query_count/* = 1*/)
{
    uint8_t const heap_id = static_cast<uint8_t>(QueryHeapType::so_statistics);
    auto& current_capacity = m_query_heap_capacities[query_heap_type_to_capacity_cache_map[heap_id][stream_output_id]];
    QueryHandle rv{ heap_id, current_capacity, query_count, static_cast<D3D12_QUERY_TYPE>(D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0 + stream_output_id) };
    current_capacity += query_count;
    return rv;
}

QueryData QueryCache::fetchQuery(QueryHandle const& query_handle) const
{
    uint64_t frame_id = m_frame_progress_tracker.lastCompletedFrameIndex() % m_settings.getMaxFramesInFlight();
    size_t offset = frame_id * m_per_frame_resolve_buffer_capacity + getHeapSegmentOffset(query_handle.heap_id);
    size_t stride = getRequiredStoragePerQuery(query_handle.heap_id);

    auto& maps = query_heap_type_to_capacity_cache_map[query_handle.heap_id];
    switch (query_handle.query_type)
    {
    case D3D12_QUERY_TYPE_OCCLUSION:
    case D3D12_QUERY_TYPE_TIMESTAMP:
    case D3D12_QUERY_TYPE_PIPELINE_STATISTICS:
    case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0:
        offset += query_handle.query_id * stride;
        break;

    case D3D12_QUERY_TYPE_BINARY_OCCLUSION:
        offset += (maps[1] + query_handle.query_id) * stride;
        break;

    case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3:
        offset += (maps[2] + query_handle.query_id) * stride;

    case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM2:
        offset += (maps[1] + query_handle.query_id) * stride;

    case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1:
        offset += (maps[0] + query_handle.query_id) * stride;
        break;

    default:
        __assume(0);
    }
    
    return QueryData{ *m_query_resolve_buffer, offset, stride * query_handle.query_count, m_resolve_buffer_mapped };
}
  
void QueryCache::initQueryCache()
{
    for (uint8_t heap_id = 0; heap_id < c_query_heap_count; ++heap_id)
    {
        m_query_heaps[heap_id].Reset();

        D3D12_QUERY_HEAP_DESC desc;
        desc.Type = toNativeQueryHeapType(heap_id);
        desc.Count = static_cast<UINT>(getHeapQueryCount(heap_id));
        desc.NodeMask = 0x1;

        if(desc.Count)
        {
            LEXGINE_THROW_ERROR_IF_FAILED(this,
                m_device.native()->CreateQueryHeap(&desc, IID_PPV_ARGS(&m_query_heaps[heap_id])),
                S_OK);
        }
    }

    m_per_frame_resolve_buffer_capacity = misc::align(getHeapSegmentOffset(c_query_heap_count), 256U);
    size_t query_buffer_required_capacity = m_per_frame_resolve_buffer_capacity * m_settings.getMaxFramesInFlight();
    bool need_reinitialize_query_resolve_buffer = m_query_resolve_buffer
        ? m_query_resolve_buffer->descriptor().width < query_buffer_required_capacity
        : true;
    if (need_reinitialize_query_resolve_buffer)
    {
        ResourceDescriptor desc = ResourceDescriptor::CreateBuffer(query_buffer_required_capacity);
        m_query_resolve_buffer.reset(new CommittedResource{ m_device, ResourceState::base_values::copy_destination,
            misc::makeEmptyOptional<ResourceOptimizedClearValue>(), desc, AbstractHeapType::readback,
            HeapCreationFlags::base_values::allow_all });
        m_query_resolve_buffer->setStringName("query_resolve_buffer");
    }
}

void QueryCache::writeFlushCommandList(CommandList const& cmd_list) const
{
    if (!m_query_resolve_buffer) return;

    auto native_command_list = cmd_list.native();
    for (uint32_t heap_id = 0; heap_id < c_query_heap_count; ++heap_id)
    {
        ID3D12QueryHeap* native_query_heap = m_query_heaps[heap_id].Get();
        if(!native_query_heap) continue;

        QueryHeapType heap_type = static_cast<QueryHeapType>(heap_id);
        size_t resolve_destination_base_offset = getHeapSegmentOffset(heap_id) +
            m_per_frame_resolve_buffer_capacity * (m_frame_progress_tracker.currentFrameIndex() % m_settings.getMaxFramesInFlight());

        switch (heap_type)
        {
        case QueryHeapType::occlusion:
        {
            D3D12_QUERY_TYPE native_query_type = D3D12_QUERY_TYPE_OCCLUSION;
            uint32_t q0_offset = m_query_heap_capacities[query_heap_type_to_capacity_cache_map[heap_id][0]];
            uint32_t q1_offset = m_query_heap_capacities[query_heap_type_to_capacity_cache_map[heap_id][1]];

            native_command_list->ResolveQueryData(native_query_heap, native_query_type, 0, q0_offset,
                m_query_resolve_buffer->native().Get(), resolve_destination_base_offset);

            native_command_list->ResolveQueryData(native_query_heap, static_cast<D3D12_QUERY_TYPE>(native_query_type + 1), static_cast<UINT>(q0_offset),
                static_cast<UINT>(q1_offset), m_query_resolve_buffer->native().Get(), 
                resolve_destination_base_offset + q0_offset * getRequiredStoragePerQuery(heap_id));
            break;
        }

        case QueryHeapType::timestamp:
        case QueryHeapType::copy_queue_timestamp:
            native_command_list->ResolveQueryData(native_query_heap, D3D12_QUERY_TYPE_TIMESTAMP, 0,
                m_query_heap_capacities[query_heap_type_to_capacity_cache_map[heap_id][0]], m_query_resolve_buffer->native().Get(),
                resolve_destination_base_offset);
            break;
        
        case QueryHeapType::pipeline_statistics:
            native_command_list->ResolveQueryData(native_query_heap, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0,
                m_query_heap_capacities[query_heap_type_to_capacity_cache_map[heap_id][0]], m_query_resolve_buffer->native().Get(),
                resolve_destination_base_offset);
            break;

        case QueryHeapType::so_statistics:
        {
            D3D12_QUERY_TYPE native_query_type = D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0;
            UINT start_query_index{ 0 };
            for (int i = 0; i < 4; ++i)
            {
                native_query_type = static_cast<D3D12_QUERY_TYPE>(native_query_type + i);
                UINT num_queries = static_cast<UINT>(m_query_heap_capacities[query_heap_type_to_capacity_cache_map[heap_id][i]]);
                native_command_list->ResolveQueryData(native_query_heap, native_query_type, start_query_index,
                    num_queries, m_query_resolve_buffer->native().Get(),
                    resolve_destination_base_offset + start_query_index * getRequiredStoragePerQuery(heap_id));
                start_query_index += num_queries;
            }
            break;
        }

        default:
            __assume(0);
        }
    }
}

size_t QueryCache::getHeapQueryCount(uint8_t heap_id) const
{
    auto& m = query_heap_type_to_capacity_cache_map[heap_id];
    return std::accumulate(m.begin(), m.end(), 0ULL,
        [this](size_t a, uint8_t map_entry) -> size_t { return a + m_query_heap_capacities[map_entry]; });
}

size_t QueryCache::getHeapSize(uint8_t heap_id) const
{
    return getHeapQueryCount(heap_id)
        * getRequiredStoragePerQuery(heap_id);
}

size_t QueryCache::getHeapSegmentOffset(uint8_t heap_id) const
{
    if (heap_id == 0) return 0;
    return getHeapSize(heap_id - 1) + getHeapSegmentOffset(heap_id - 1);
}


    
