#include "engine/core/globals.h"
#include "engine/core/exception.h"

#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/d3d12/heap.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/frame_progress_tracker.h"
#include "upload_buffer_allocator.h"


namespace lexgine::core::dx::d3d12
{

size_t UploadDataBlock::capacity() const
{
    return m_allocation_end - m_allocation_begin;
}

bool UploadDataBlock::isInUse() const
{
    return m_allocator.completedWork() < m_controlling_signal_value;
}

void* UploadDataBlock::cpuAddress() const
{
    return m_buffer_cpu_addr + m_allocation_begin;
}

void* UploadDataBlock::offsettedCpuAddress(uint32_t offset) const
{
    assert(m_allocation_begin + offset < m_allocation_end);
    return m_buffer_cpu_addr + m_allocation_begin + offset;
}

uint64_t UploadDataBlock::virtualGpuAddress() const
{
    return m_buffer_virtual_gpu_addr + m_allocation_begin;
}

uint64_t UploadDataBlock::offsettedVirtualGpuAddress(uint32_t offset) const
{
    return virtualGpuAddress() + offset;
}

uint32_t UploadDataBlock::offset() const
{
    return m_allocation_begin;
}

UploadDataBlock::UploadDataBlock(UploadDataAllocator const& allocator,
    void* buffer_cpu_addr, uint64_t buffer_virtual_gpu_addr,
    uint64_t signal_value, uint32_t allocation_begin, uint32_t allocation_end)
    : m_allocator{ allocator }
    , m_buffer_cpu_addr{ static_cast<unsigned char*>(buffer_cpu_addr) }
    , m_buffer_virtual_gpu_addr{ buffer_virtual_gpu_addr }
    , m_controlling_signal_value{ signal_value }
    , m_allocation_begin{ allocation_begin }
    , m_allocation_end{ allocation_end }
{
}



UploadDataAllocator::UploadDataAllocator(Globals& globals,
    uint64_t offset_from_heap_start, size_t upload_buffer_size)
    : m_upload_heap{ globals.get<DxResourceFactory>()->retrieveUploadHeap(*globals.get<Device>()) }
    , m_upload_buffer{ m_upload_heap, offset_from_heap_start, ResourceState::base_values::generic_read,
        misc::Optional<ResourceOptimizedClearValue>{}, ResourceDescriptor::CreateBuffer(upload_buffer_size) }
    , m_max_non_blocking_allocation_timeout{ globals.get<GlobalSettings>()->getMaxNonBlockingUploadBufferAllocationTimeout() }
    , m_buffer_size{ upload_buffer_size }
    , m_unpartitioned_chunk_size{ upload_buffer_size }
{
    assert(offset_from_heap_start + upload_buffer_size <= m_upload_heap.capacity());

    m_upload_buffer_cpu_address = m_upload_buffer.map();
    m_upload_buffer_gpu_virtual_address = m_upload_buffer.getGPUVirtualAddress();

    m_blocks.emplace_back(allocation_bucket{});
    m_allocation_hint = { m_blocks.begin(), m_blocks.back().begin() };
}

UploadDataAllocator::~UploadDataAllocator()
{
    m_upload_buffer.unmap();
}

UploadDataAllocator::address_type UploadDataAllocator::allocate(size_t size_in_bytes, bool is_blocking_call)
{
    assert(size_in_bytes != 0);

    std::lock_guard<std::mutex> lock{ m_access_semaphore };
    
    // try to allocate space in unpartitioned part of the buffer
    if (m_unpartitioned_chunk_size >= size_in_bytes)
    {
        uint32_t new_allocation_start = m_buffer_size - m_unpartitioned_chunk_size;
        uint32_t new_allocation_end = static_cast<uint32_t>(misc::align(new_allocation_start + size_in_bytes, 256));

        memory_block_type& new_memory_block = allocateNewMemoryBlock(new_allocation_start, new_allocation_end);
        
        if (new_allocation_end <= m_unpartitioned_chunk_size)
        {
            m_unpartitioned_chunk_size -= new_allocation_end;
        } else 
        {
            m_unpartitioned_chunk_size = 0;
        }

        return { &new_memory_block };
    }

    // Attempt to allocate space at current allocation hint
    address_type result = allocateInternal(size_in_bytes, is_blocking_call);
    if (result) {
        return result;
    }

    // Allocation failed. Reset allocation hint and try allocating space at the beginning of the buffer
    m_allocation_hint.bucket_iter = m_blocks.begin();
    m_allocation_hint.block_iter = m_blocks.front().begin();
    result = allocateInternal(size_in_bytes, is_blocking_call);
    if (result) {
        return result;
    }

    return { nullptr };
}

uint64_t UploadDataAllocator::completedWork() const
{
    return lastSignaledValueOfControllingSignal();
}

uint64_t UploadDataAllocator::scheduledWork() const
{
    return nextValueOfControllingSignal() - 1;
}

size_t UploadDataAllocator::getPartitionsCount() const
{
    return std::accumulate(m_blocks.begin(), m_blocks.end(), static_cast<size_t>(0),
        [](size_t accumulator, allocation_bucket const& block) { return accumulator + block.size(); });
}

Resource const& UploadDataAllocator::getUploadResource() const
{
    return m_upload_buffer;
}

UploadDataAllocator::address_type UploadDataAllocator::allocateInternal(size_t size_in_bytes, bool is_blocking_call)
{
    if (m_allocation_hint.bucket_iter->empty() || m_allocation_hint.block()->m_controlling_signal_value == nextValueOfControllingSignal()) {
        // Unable to allocate more space, the buffer is exhausted
        LEXGINE_LOG_ERROR(this,
            "Unable to allocate block in upload buffer. Requested size of the allocation ("
                + std::to_string(size_in_bytes) + " bytes) exceeds the remaining capacity of the buffer ("
                + std::to_string(m_unpartitioned_chunk_size) + " bytes with total of " + std::to_string(m_fragmentation_capacity)
                + " fragmented bytes)");

        return { nullptr };
    }

    // attempt to allocate space within the "allocation hint" (wrap-around mode)

    size_t space_retrieved{ 0 };
    uint32_t trailing_end_allocation_addr{ m_allocation_hint.block()->m_allocation_begin };
    uint64_t controlling_signal_value{ 0 };

    list_of_allocation_buckets::iterator p{};
    allocation_bucket::iterator q{};
    for (p = m_allocation_hint.bucket_iter; p != m_blocks.end(); ++p) {
        for (q = (p == m_allocation_hint.bucket_iter ? m_allocation_hint.block_iter : p->begin());
            space_retrieved < size_in_bytes && q != p->end();
            ++q) {
            memory_block_type& memory_block = *q;
            if (memory_block->m_controlling_signal_value == nextValueOfControllingSignal())
            {
                break;
            }

            size_t current_end_addr = memory_block->m_allocation_end;
            space_retrieved += current_end_addr - trailing_end_allocation_addr;
            m_fragmentation_capacity -= trailing_end_allocation_addr - memory_block->m_allocation_begin;
            trailing_end_allocation_addr = current_end_addr;
            controlling_signal_value = (std::max)(controlling_signal_value, memory_block->m_controlling_signal_value);
        }

        if (space_retrieved >= size_in_bytes) {
            break;
        }

        if ((*q)->m_controlling_signal_value == nextValueOfControllingSignal()) {
            break;
        }
    }

    if (p == m_blocks.end()) {
        // all blocks seem to allow retirement, try to combine their space with unpartitioned remainder of the buffer
        space_retrieved += m_unpartitioned_chunk_size;
    }

    if (space_retrieved >= size_in_bytes) {
        // allocation after the last block succeeded

        if (is_blocking_call) {
            waitUntilControllingSignalValue(controlling_signal_value);
        }
        else if (!waitUntilControllingSignalValue(controlling_signal_value, m_max_non_blocking_allocation_timeout)) {
            return { nullptr };
        }

        memory_block_type& reused_memory_block = m_allocation_hint.block();

        size_t end_of_new_allocation = reused_memory_block->m_allocation_begin + static_cast<uint32_t>(misc::align(size_in_bytes, 256));
        reused_memory_block->m_allocation_end = end_of_new_allocation;
        reused_memory_block->m_controlling_signal_value = nextValueOfControllingSignal();

        ++m_allocation_hint.block_iter;
        if (m_allocation_hint.block_iter == m_allocation_hint.bucket_iter->end()) {
            ++m_allocation_hint.bucket_iter;
            if (m_allocation_hint.bucket_iter != m_blocks.end()) {
                m_allocation_hint.block_iter = m_allocation_hint.bucket_iter->begin();
            }
        }

        if (trailing_end_allocation_addr > end_of_new_allocation) {
            // some unpartitioned space remained, attempt to track it within one of retired memory blocks
            if (m_allocation_hint.bucket_iter != m_blocks.end()) {
                memory_block_type& memory_block = m_allocation_hint.block();
                if (memory_block->m_controlling_signal_value <= lastSignaledValueOfControllingSignal()) {
                    memory_block->m_allocation_begin = end_of_new_allocation;
                    memory_block->m_allocation_end = (std::max)(trailing_end_allocation_addr, memory_block->m_allocation_end);
                }
                else {
                    m_fragmentation_capacity += trailing_end_allocation_addr - end_of_new_allocation;
                }
            }
            else {
                // trailing memory partitions are exhausted, add the remaining space to unpartitioned chunk
                m_unpartitioned_chunk_size += trailing_end_allocation_addr - end_of_new_allocation;
            }
        }

        if (m_allocation_hint.bucket_iter == m_blocks.end()) {
            // wrap around
            m_allocation_hint.bucket_iter = m_blocks.begin();
            m_allocation_hint.block_iter = m_blocks.front().begin();
        }

        return { &reused_memory_block };
    }


    return { nullptr };
}

UploadDataAllocator::memory_block_type& UploadDataAllocator::allocateNewMemoryBlock(uint32_t allocation_begin_address, uint32_t allocation_end_address)
{
    if (m_blocks.back().exhausted())
    {
        m_blocks.emplace_back(allocation_bucket{});
    }
    allocation_bucket& last_bucket = m_blocks.back();
    last_bucket.emplace_back(*this, m_upload_buffer_cpu_address, m_upload_buffer_gpu_virtual_address,
        nextValueOfControllingSignal(), allocation_begin_address, allocation_end_address);

    return last_bucket.back();
}


DedicatedUploadDataStreamAllocator::DedicatedUploadDataStreamAllocator(Globals& globals, uint64_t offset_from_heap_start, size_t upload_buffer_size)
    : UploadDataAllocator{ globals, offset_from_heap_start, upload_buffer_size }
    , m_progress_tracking_signal{ *globals.get<Device>(), FenceSharing::none }
{

}

void DedicatedUploadDataStreamAllocator::signalAllocator(CommandQueue const& signalling_queue)
{
    m_progress_tracking_signal.signalFromGPU(signalling_queue);
}

void DedicatedUploadDataStreamAllocator::waitUntilControllingSignalValue(uint64_t value) const
{
    m_progress_tracking_signal.waitUntilValue(value);
}

bool DedicatedUploadDataStreamAllocator::waitUntilControllingSignalValue(uint64_t value, uint32_t timeout_in_milliseconds) const
{
    return m_progress_tracking_signal.waitUntilValue(value, timeout_in_milliseconds);
}

uint64_t DedicatedUploadDataStreamAllocator::nextValueOfControllingSignal() const
{
    return m_progress_tracking_signal.nextValueOfSignal();
}

uint64_t DedicatedUploadDataStreamAllocator::lastSignaledValueOfControllingSignal() const
{
    return m_progress_tracking_signal.lastValueSignaled();
}

PerFrameUploadDataStreamAllocator::PerFrameUploadDataStreamAllocator(Globals& globals, uint64_t offset_from_heap_start, uint64_t upload_buffer_size,
    FrameProgressTracker const& frame_progress_tracker)
    : UploadDataAllocator{ globals, offset_from_heap_start, upload_buffer_size }
    , m_frame_progress_tracker{ frame_progress_tracker }
{
}

FrameProgressTracker const& PerFrameUploadDataStreamAllocator::frameProgressTracker() const
{
    return m_frame_progress_tracker;
}

void PerFrameUploadDataStreamAllocator::waitUntilControllingSignalValue(uint64_t value) const
{
    if (value <= m_frame_progress_tracker.completedFramesCount()) return;
    m_frame_progress_tracker.waitForFrameCompletion(value - 1);
}

bool PerFrameUploadDataStreamAllocator::waitUntilControllingSignalValue(uint64_t value, uint32_t timeout_in_milliseconds) const
{
    if (value <= m_frame_progress_tracker.completedFramesCount()) return true;
    return m_frame_progress_tracker.waitForFrameCompletion(value - 1, timeout_in_milliseconds);
}

uint64_t PerFrameUploadDataStreamAllocator::nextValueOfControllingSignal() const
{
    return m_frame_progress_tracker.currentFrameIndex() + 1;
}

uint64_t PerFrameUploadDataStreamAllocator::lastSignaledValueOfControllingSignal() const
{
    return m_frame_progress_tracker.completedFramesCount();
}

}