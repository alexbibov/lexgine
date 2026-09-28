#include <cassert>

#include "engine/core/exception.h"
#include "engine/core/global_settings.h"
#include "engine/core/dx/d3d12/cbv_descriptor.h"
#include "engine/core/dx/d3d12/srv_descriptor.h"
#include "engine/core/dx/d3d12/uav_descriptor.h"

#include "descriptor_allocator.h"


namespace lexgine::core::dx::d3d12 {

DescriptorAllocator::DescriptorAllocator(DescriptorHeap& descriptor_heap)
    : m_descriptor_heap{ descriptor_heap }
{

}

DescriptorTable DescriptorAllocator::allocateDescriptorTable(uint32_t capacity)
{
    uint32_t offset = reserveDescriptors(capacity);
    uint32_t descriptor_size = m_descriptor_heap.getDescriptorSize();
    return {
        .offset = offset,
        .cpu_pointer = m_descriptor_heap.getBaseCPUPointer() + static_cast<size_t>(descriptor_size) * offset,
        .gpu_pointer = m_descriptor_heap.getBaseGPUPointer() + static_cast<uint64_t>(descriptor_size) * offset,
        .descriptor_count = capacity,
        .descriptor_size = descriptor_size,
        .p_heap = &m_descriptor_heap
    };
}

uint64_t DescriptorAllocator::createConstantBufferViewDescriptors(std::vector<CBVDescriptor> const& cbv_descriptors)
{
    uint32_t offset = reserveDescriptors(static_cast<uint32_t>(cbv_descriptors.size()));
    return m_descriptor_heap.createConstantBufferViewDescriptors(offset, cbv_descriptors);
}

uint64_t DescriptorAllocator::createShaderResourceViewDescriptors(std::vector<SRVDescriptor> const& srv_descriptors)
{
    uint32_t offset = reserveDescriptors(static_cast<uint32_t>(srv_descriptors.size()));
    return m_descriptor_heap.createShaderResourceViewDescriptors(offset, srv_descriptors);
}

uint64_t DescriptorAllocator::createUnorderedAccessViewDescriptors(std::vector<UAVDescriptor> const& uav_descriptors)
{
    uint32_t offset = reserveDescriptors(static_cast<uint32_t>(uav_descriptors.size()));
    return m_descriptor_heap.createUnorderedAccessViewDescriptors(offset, uav_descriptors);
}


PersistentDescriptorAllocator::PersistentDescriptorAllocator(DescriptorHeap& descriptor_heap, uint32_t region_offset, uint32_t region_size)
    : DescriptorAllocator{ descriptor_heap }
    , m_region_offset{ region_offset }
    , m_region_size{ region_size }
    , m_allocation_offset{ 0 }
{
    assert(static_cast<uint64_t>(m_region_offset) + m_region_size <= descriptor_heap.capacity());
}

void PersistentDescriptorAllocator::reset()
{
    m_allocation_offset.store(0, std::memory_order_relaxed);
}

uint32_t PersistentDescriptorAllocator::reserveDescriptors(uint32_t descriptor_count)
{
    uint32_t first_reserved_slot = m_allocation_offset.fetch_add(descriptor_count);
    if (static_cast<uint64_t>(first_reserved_slot) + descriptor_count > m_region_size)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(*this,
            "Unable to reserve " + std::to_string(descriptor_count) + " persistent descriptors in descriptor heap \""
            + descriptorHeap().getStringName() + "\": the persistent region is exhausted");
    }

    return m_region_offset + first_reserved_slot;
}


TransientDescriptorAllocator::TransientDescriptorAllocator(DescriptorHeap& descriptor_heap, uint32_t region_offset, uint32_t region_size,
    GlobalSettings const& global_settings)
    : DescriptorAllocator{ descriptor_heap }
    , m_region_offset{ region_offset }
    , m_page_size{ region_size }
    , m_frames_in_flight{ global_settings.getMaxFramesInFlight() }
    , m_active_frame_index{ 0 }
    , m_per_frame_allocation_offset(m_frames_in_flight)
{
    assert(static_cast<uint64_t>(m_region_offset) + static_cast<uint64_t>(m_page_size) * m_frames_in_flight <= descriptor_heap.capacity());
}

void TransientDescriptorAllocator::nextFrame()
{
    m_active_frame_index = (m_active_frame_index + 1) % m_frames_in_flight;
    m_per_frame_allocation_offset[m_active_frame_index].store(0, std::memory_order_relaxed);
}

void TransientDescriptorAllocator::reset()
{
    for (std::atomic_uint32_t& offset : m_per_frame_allocation_offset)
    {
        offset.store(0, std::memory_order_relaxed);
    }
}

uint32_t TransientDescriptorAllocator::reserveDescriptors(uint32_t descriptor_count)
{
    uint32_t first_reserved_slot = m_per_frame_allocation_offset[m_active_frame_index].fetch_add(descriptor_count);
    if (static_cast<uint64_t>(first_reserved_slot) + descriptor_count > m_page_size)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(*this,
            "Unable to reserve " + std::to_string(descriptor_count) + " transient descriptors in descriptor heap \""
            + descriptorHeap().getStringName() + "\": the page of frame #" + std::to_string(m_active_frame_index) + " is exhausted");
    }

    return m_region_offset + m_page_size * m_active_frame_index + first_reserved_slot;
}

}
