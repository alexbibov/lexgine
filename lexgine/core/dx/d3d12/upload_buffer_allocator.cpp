#include "upload_buffer_allocator.h"

#include "lexgine/core/globals.h"
#include "lexgine/core/dx/d3d12/dx_resource_factory.h"
#include "lexgine/core/dx/d3d12/heap.h"
#include "lexgine/core/dx/d3d12/device.h"
#include "lexgine/core/exception.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

size_t UploadBufferBlock::capacity() const
{
    return m_allocation_end - m_allocation_begin;
}

bool UploadBufferBlock::isInUse() const
{
    return m_tracking_signal.lastValueSignaled() < m_controlling_signal_value;
}

void* UploadBufferBlock::address() const
{
    return m_mapped_gpu_buffer_addr + m_allocation_begin;
}

void* UploadBufferBlock::offsetted_address(uint32_t offset) const
{
    assert(m_allocation_begin + offset < m_allocation_end);
    return m_mapped_gpu_buffer_addr + m_allocation_begin + offset;
}

uint32_t UploadBufferBlock::offset() const
{
    return m_allocation_begin;
}

UploadBufferBlock::UploadBufferBlock(Signal const& tracking_signal, void* mapped_gpu_buffer_addr,
    uint64_t signal_value, uint32_t allocation_begin, uint32_t allocation_end):
    m_tracking_signal{ tracking_signal },
    m_mapped_gpu_buffer_addr{ static_cast<unsigned char*>(mapped_gpu_buffer_addr) },
    m_controlling_signal_value{ signal_value },
    m_allocation_begin{ allocation_begin },
    m_allocation_end{ allocation_end }
{
}



UploadBufferAllocator::UploadBufferAllocator(Globals& globals, 
    uint64_t offset_from_heap_start, size_t upload_buffer_size) :
    m_dx_resource_factory{ *globals.get<DxResourceFactory>() },
    m_device{ *globals.get<Device>() },
    m_upload_heap{ m_dx_resource_factory.retrieveUploadHeap(m_device) },
    m_progress_tracking_signal{ m_device, FenceSharing::none },
    m_upload_buffer{ m_upload_heap, offset_from_heap_start, ResourceState::enum_type::generic_read,
        misc::makeEmptyOptional<ResourceOptimizedClearValue>(), ResourceDescriptor::CreateBuffer(upload_buffer_size) },
    m_unpartitioned_chunk_size{ upload_buffer_size },
    m_max_non_blocking_allocation_timeout{ globals.get<GlobalSettings>()->getMaxNonBlockingUploadBufferAllocationTimeout() }
{
    assert(offset_from_heap_start + upload_buffer_size <= m_upload_heap.capacity());

    m_upload_buffer_mapping = m_upload_buffer.map();
}

UploadBufferAllocator::~UploadBufferAllocator()
{
    m_upload_buffer.unmap();
}

UploadBufferAllocator::address_type UploadBufferAllocator::allocate(size_t size_in_bytes, bool is_blocking_call)
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
        end_of_new_allocation = static_cast<uint32_t>(allocation_begin + size_in_bytes);
        uint32_t allocation_end = end_of_new_allocation;

        m_blocks.emplace_back(m_progress_tracking_signal, m_upload_buffer_mapping,
            m_progress_tracking_signal.nextValueOfSignal(),
            allocation_begin, allocation_end);

        m_unpartitioned_chunk_size -= size_in_bytes;

        m_last_allocation = --m_blocks.end();
        allocation_successful = true;
    }

    // attempt to allocate space after the last allocation
    if(!allocation_successful)
    {
        size_t requested_space_counter{ 0U };
        uint32_t trailing_end_allocation_addr{ (*m_last_allocation)->m_allocation_end };
        uint64_t controlling_signal_value{ 0U };

        auto q = ++m_last_allocation;
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

            if (is_blocking_call) m_progress_tracking_signal.waitUntilValue(controlling_signal_value);
            else m_progress_tracking_signal.waitUntilValue(controlling_signal_value, m_max_non_blocking_allocation_timeout);

            auto next_allocation = ++m_last_allocation;
            (*next_allocation)->m_allocation_begin = (*m_last_allocation)->m_allocation_end;
            end_of_new_allocation = (*next_allocation)->m_allocation_end = 
                static_cast<uint32_t>((*next_allocation)->m_allocation_begin + size_in_bytes);
            (*next_allocation)->m_controlling_signal_value = m_progress_tracking_signal.nextValueOfSignal();

            m_unpartitioned_chunk_size -= size_in_bytes - requested_space_counter;
            m_last_allocation = next_allocation;
            first_allocation_block_to_invalidate = ++next_allocation;
            one_past_last_allocation_block_to_invalidate = q;
            allocation_successful = true;
        }
    }
    
    // attempt to allocate space before the last allocation
    if(!allocation_successful)
    {
        size_t requested_size_in_bytes{ 0U };
        uint32_t trailing_end_allocation_addr{ 0U };
        uint64_t controlling_signal_value{ 0U };

        auto q = m_blocks.begin();
        while (q != m_last_allocation && requested_size_in_bytes < size_in_bytes)
        {
            requested_size_in_bytes += (*q)->m_allocation_end - trailing_end_allocation_addr;
            trailing_end_allocation_addr = (*q)->m_allocation_end;
            controlling_signal_value = (std::max)(controlling_signal_value, (*q)->m_controlling_signal_value);
            ++q;
        }

        if (requested_size_in_bytes >= size_in_bytes)
        {
            if (is_blocking_call) m_progress_tracking_signal.waitUntilValue(controlling_signal_value);
            else m_progress_tracking_signal.waitUntilValue(controlling_signal_value, m_max_non_blocking_allocation_timeout);

            auto next_allocation = m_blocks.begin();
            end_of_new_allocation = (*next_allocation)->m_allocation_end = 
                static_cast<uint32_t>(size_in_bytes);
            (*next_allocation)->m_controlling_signal_value = m_progress_tracking_signal.nextValueOfSignal();

            m_last_allocation = next_allocation;
            first_allocation_block_to_invalidate = ++next_allocation;
            one_past_last_allocation_block_to_invalidate = q;
            allocation_successful = true;
        }
    }

    // the most recent allocation took too much space, need to wait until all allocations made in the buffer
    // are not any longer in use
    if (!allocation_successful)
    {
        if (is_blocking_call) m_progress_tracking_signal.waitUntilValue((*m_last_allocation)->m_controlling_signal_value);
        else m_progress_tracking_signal.waitUntilValue((*m_last_allocation)->m_controlling_signal_value, m_max_non_blocking_allocation_timeout);

        m_last_allocation = m_blocks.begin();
        end_of_new_allocation = (*m_last_allocation)->m_allocation_end 
            = static_cast<uint32_t>(size_in_bytes);
        (*m_last_allocation)->m_controlling_signal_value = m_progress_tracking_signal.nextValueOfSignal();

        first_allocation_block_to_invalidate = ++m_last_allocation;
        one_past_last_allocation_block_to_invalidate = m_blocks.end();
        allocation_successful = true;
    }
    
    assert(allocation_successful);

    // invalidate unused allocation blocks
    std::for_each(first_allocation_block_to_invalidate, one_past_last_allocation_block_to_invalidate,
        [end_of_new_allocation](memory_block_type& mem_block_ref)
        {
            mem_block_ref->m_allocation_begin = mem_block_ref->m_allocation_end =
                static_cast<uint32_t>(end_of_new_allocation);
        }
    );

    m_unpartitioned_chunk_size = m_upload_buffer.descriptor().width - m_blocks.back()->m_allocation_end;
    
    memory_block_type& mem_block_ref = *m_last_allocation;
    return address_type{ &mem_block_ref };

}

void UploadBufferAllocator::signalAllocator(CommandQueue const& signalling_queue) const
{
    m_progress_tracking_signal.signalFromGPU(signalling_queue);
}

uint64_t UploadBufferAllocator::completedWork() const
{
    return m_progress_tracking_signal.lastValueSignaled();
}

uint64_t UploadBufferAllocator::scheduledWork() const
{
    return m_progress_tracking_signal.nextValueOfSignal() - 1;
}

uint64_t UploadBufferAllocator::totalCapacity() const
{
    return m_upload_buffer.descriptor().width;
}

PlacedResource const& UploadBufferAllocator::getUploadResource() const
{
    return m_upload_buffer;
}
