#include "engine/core/globals.h"
#include "engine/core/exception.h"

#include "dx_resource_factory.h"
#include "heap.h"
#include "device.h"
#include "frame_progress_tracker.h"

#include "upload_buffer_allocator.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

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
        misc::makeEmptyOptional<ResourceOptimizedClearValue>(), ResourceDescriptor::CreateBuffer(upload_buffer_size) }
    , m_last_allocation{ m_blocks.end() }
    , m_unpartitioned_chunk_size{ upload_buffer_size }
    , m_max_non_blocking_allocation_timeout{ globals.get<GlobalSettings>()->getMaxNonBlockingUploadBufferAllocationTimeout() }
{
    assert(offset_from_heap_start + upload_buffer_size <= m_upload_heap.capacity());

    m_upload_buffer_mapping = m_upload_buffer.map();
    m_upload_buffer_gpu_virtual_address = m_upload_buffer.getGPUVirtualAddress();

    m_blocks.reserve(200);
}

UploadDataAllocator::~UploadDataAllocator()
{
    m_upload_buffer.unmap();
}

UploadDataAllocator::address_type UploadDataAllocator::allocate(size_t size_in_bytes, bool is_blocking_call)
{
    if (size_in_bytes > m_upload_buffer.descriptor().width)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
            "Unable to allocate block in upload buffer. Requested size of the allocation ("
            + std::to_string(size_in_bytes) + " bytes) exceeds the total size of the buffer ("
            + std::to_string(m_upload_buffer.descriptor().width) + " bytes)");
    }


    std::lock_guard<std::mutex> lock{ m_access_semaphore };

    bool allocation_successful{ false };
    uint32_t end_of_new_allocation{ 0U };
    auto first_allocation_block_to_invalidate = m_blocks.begin();
    auto one_past_last_allocation_block_to_invalidate = m_blocks.begin();

    // try to allocate space in the end of the buffer
    if (m_unpartitioned_chunk_size >= size_in_bytes)
    {
        uint32_t allocation_begin = m_blocks.size() ? m_blocks.back()->m_allocation_end : 0U;
        end_of_new_allocation = static_cast<uint32_t>(misc::align(allocation_begin + size_in_bytes, 256));
        uint32_t allocation_end = end_of_new_allocation;

        m_blocks.emplace_back(*this, m_upload_buffer_mapping, m_upload_buffer_gpu_virtual_address,
            nextValueOfControllingSignal(),
            allocation_begin, allocation_end);

        m_unpartitioned_chunk_size -= size_in_bytes;

        m_last_allocation = std::prev(m_blocks.end());
        allocation_successful = true;
    }

    // attempt to allocate space after the last allocation (the wrap-around mode)
    if (!allocation_successful)
    {
        size_t requested_space_counter{ 0U };
        uint32_t trailing_end_allocation_addr{ (*m_last_allocation)->m_allocation_end };
        uint64_t controlling_signal_value{ 0U };

        auto q = std::next(m_last_allocation);
        while (q != m_blocks.end() && requested_space_counter < size_in_bytes)
        {
            requested_space_counter += (*q)->m_allocation_end - trailing_end_allocation_addr;
            trailing_end_allocation_addr = (*q)->m_allocation_end;
            controlling_signal_value = (std::max)(controlling_signal_value, (*q)->m_controlling_signal_value);
            ++q;
        }

        if (requested_space_counter + m_unpartitioned_chunk_size >= size_in_bytes)
        {
            // allocation after the last block succeeded

            if (is_blocking_call) waitUntilControllingSignalValue(controlling_signal_value);
            else waitUntilControllingSignalValue(controlling_signal_value, m_max_non_blocking_allocation_timeout);

            auto next_allocation = std::next(m_last_allocation);
            (*next_allocation)->m_allocation_begin = (*m_last_allocation)->m_allocation_end;
            end_of_new_allocation = (*next_allocation)->m_allocation_end =
                static_cast<uint32_t>(misc::align((*next_allocation)->m_allocation_begin + size_in_bytes, 256));
            (*next_allocation)->m_controlling_signal_value = nextValueOfControllingSignal();

            m_unpartitioned_chunk_size -= size_in_bytes - requested_space_counter;
            m_last_allocation = next_allocation;
            first_allocation_block_to_invalidate = ++next_allocation;
            one_past_last_allocation_block_to_invalidate = q;
            allocation_successful = true;
        }
    }

    // attempt to allocate space before the last allocation
    if (!allocation_successful)
    {
        size_t requested_space_counter{ 0U };
        uint32_t trailing_end_allocation_addr{ 0U };
        uint64_t controlling_signal_value{ 0U };

        auto q = m_blocks.begin();
        while (q != m_last_allocation && requested_space_counter < size_in_bytes)
        {
            requested_space_counter += (*q)->m_allocation_end - trailing_end_allocation_addr;
            trailing_end_allocation_addr = (*q)->m_allocation_end;
            controlling_signal_value = (std::max)(controlling_signal_value, (*q)->m_controlling_signal_value);
            ++q;
        }

        if (requested_space_counter >= size_in_bytes)
        {
            if (is_blocking_call) waitUntilControllingSignalValue(controlling_signal_value);
            else waitUntilControllingSignalValue(controlling_signal_value, m_max_non_blocking_allocation_timeout);

            auto next_allocation = m_blocks.begin();
            end_of_new_allocation = (*next_allocation)->m_allocation_end = static_cast<uint32_t>(misc::align(size_in_bytes, 256));
            (*next_allocation)->m_controlling_signal_value = nextValueOfControllingSignal();

            m_last_allocation = next_allocation;
            first_allocation_block_to_invalidate = std::next(next_allocation);
            one_past_last_allocation_block_to_invalidate = q;
            allocation_successful = true;
        }
    }

    // the most recent allocation took too much space, need to wait until all allocations made in the buffer
    // are not any longer in use
    if (!allocation_successful)
    {
        if (is_blocking_call) waitUntilControllingSignalValue((*m_last_allocation)->m_controlling_signal_value);
        else waitUntilControllingSignalValue((*m_last_allocation)->m_controlling_signal_value, m_max_non_blocking_allocation_timeout);

        m_last_allocation = m_blocks.begin();
        end_of_new_allocation = (*m_last_allocation)->m_allocation_end = static_cast<uint32_t>(misc::align(size_in_bytes, 256));
        (*m_last_allocation)->m_controlling_signal_value = nextValueOfControllingSignal();

        first_allocation_block_to_invalidate = std::next(m_last_allocation);
        one_past_last_allocation_block_to_invalidate = m_blocks.end();
        allocation_successful = true;
    }

    assert(allocation_successful);

    // invalidate unused allocation blocks
    std::for_each(first_allocation_block_to_invalidate, one_past_last_allocation_block_to_invalidate,
        [end_of_new_allocation](memory_block_type& mem_block_ref)
        {
            mem_block_ref->m_allocation_begin = mem_block_ref->m_allocation_end = static_cast<uint32_t>(end_of_new_allocation);
        }
    );

    m_unpartitioned_chunk_size = m_upload_buffer.descriptor().width - m_blocks.back()->m_allocation_end;

    memory_block_type& mem_block_ref = *m_last_allocation;
    return address_type{ &mem_block_ref };

}

uint64_t UploadDataAllocator::completedWork() const
{
    return lastSignaledValueOfControllingSignal();
}

uint64_t UploadDataAllocator::scheduledWork() const
{
    return nextValueOfControllingSignal() - 1;
}

uint64_t UploadDataAllocator::totalCapacity() const
{
    return m_upload_buffer.descriptor().width;
}

Resource const& UploadDataAllocator::getUploadResource() const
{
    return m_upload_buffer;
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

void DedicatedUploadDataStreamAllocator::waitUntilControllingSignalValue(uint64_t value, uint32_t timeout_in_milliseconds) const
{
    m_progress_tracking_signal.waitUntilValue(value, timeout_in_milliseconds);
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

void PerFrameUploadDataStreamAllocator::waitUntilControllingSignalValue(uint64_t value, uint32_t timeout_in_milliseconds) const
{
    if (value <= m_frame_progress_tracker.completedFramesCount()) return;
    m_frame_progress_tracker.waitForFrameCompletion(value - 1, timeout_in_milliseconds);
}

uint64_t PerFrameUploadDataStreamAllocator::nextValueOfControllingSignal() const
{
    return m_frame_progress_tracker.currentFrameIndex() + 1;
}

uint64_t PerFrameUploadDataStreamAllocator::lastSignaledValueOfControllingSignal() const
{
    return m_frame_progress_tracker.completedFramesCount();
}
