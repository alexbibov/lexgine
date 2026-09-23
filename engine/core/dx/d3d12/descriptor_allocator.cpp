#include "engine/core/global_settings.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/d3d12/device.h"

#include "descriptor_allocator.h"


namespace lexgine::core::dx::d3d12 {

DescriptorAllocator::DescriptorAllocator(Globals& globals)
    : ProvidesGlobals{ globals }
    , m_descriptor_heap{ globals.dxResourceFactory().retrieveDescriptorHeap(globals.device(), DescriptorHeapType::cbv_srv_uav) }
    , m_frame_progress_tracker{ globals.device().frameProgressTracker() }
    , m_frames_in_flight{ globals.globalSettings().getMaxFramesInFlight() }
    , m_persistent_page_size{ globals.globalSettings().getCbvSrvUavDescriptorHeapPersistentPartitionCapacity() }
    , m_transient_page_size{ globals.globalSettings().getCbvSrvUavDescriptorHeapDynamicPartitionCapacity() }
    , m_active_frame_index{ m_frame_progress_tracker.currentFrameIndex() }
    , m_static_allocation_offset{ 0 }
    , m_per_frame_transient_allocation_offset(m_frames_in_flight)
{

}

uint64_t DescriptorAllocator::createConstantBufferViewDescriptors(std::vector<CBVDescriptor> const& cbv_descriptors, DescriptorLifetime lifetime)
{
    std::optional<size_t> offset = reserveDescriptors(static_cast<uint32_t>(cbv_descriptors.size()), lifetime);
    if (!offset)
    {
        return 0;
    }

    return m_descriptor_heap.createConstantBufferViewDescriptors(*offset, cbv_descriptors);
}

uint64_t DescriptorAllocator::createShaderResourceViewDescriptors(std::vector<SRVDescriptor> const& srv_descriptors, DescriptorLifetime lifetime)
{
    std::optional<size_t> offset = reserveDescriptors(static_cast<uint32_t>(srv_descriptors.size()), lifetime);
    if (!offset)
    {
        return 0;
    }

    return m_descriptor_heap.createShaderResourceViewDescriptors(*offset, srv_descriptors);
}

uint64_t DescriptorAllocator::createUnorderedAccessViewDescriptors(std::vector<UAVDescriptor> const& uav_descriptors, DescriptorLifetime lifetime)
{
    std::optional<size_t> offset = reserveDescriptors(static_cast<uint32_t>(uav_descriptors.size()), lifetime);
    if (!offset)
    {
        return 0;
    }

    return m_descriptor_heap.createUnorderedAccessViewDescriptors(*offset, uav_descriptors);
}

std::optional<size_t> DescriptorAllocator::reserveDescriptors(uint32_t descriptor_count, DescriptorLifetime lifetime)
{
    switch (lifetime)
    {
    case DescriptorLifetime::persistent:
    {
        uint32_t first_reserved_slot = m_static_allocation_offset.fetch_add(descriptor_count);
        if (first_reserved_slot + descriptor_count > m_persistent_page_size)
        {
            logger().out("Persistent descriptor heap page is exhausted", misc::LogMessageType::critical);
            return std::nullopt;
        }
        return first_reserved_slot;
    }
    case DescriptorLifetime::transient:
    {
        uint32_t first_reserved_slot = m_per_frame_transient_allocation_offset[m_active_frame_index].fetch_add(descriptor_count);
        if (first_reserved_slot + descriptor_count > m_transient_page_size)
        {
            logger().out("Transient descriptor heap page is exhausted", misc::LogMessageType::critical);
            return std::nullopt;
        }
        return static_cast<size_t>(m_persistent_page_size) + static_cast<size_t>(m_transient_page_size) * m_active_frame_index + first_reserved_slot;
    }
    default:
        LEXGINE_ASSUME;
    }
}

void DescriptorAllocator::nextFrame()
{
    m_active_frame_index = (m_active_frame_index + 1) % m_frames_in_flight;
    m_per_frame_transient_allocation_offset[m_active_frame_index].store(0, std::memory_order_relaxed);
}

void DescriptorAllocator::reset(DescriptorLifetime lifetime)
{
    switch (lifetime)
    {
    case DescriptorLifetime::persistent:
        m_static_allocation_offset.store(0, std::memory_order_relaxed);
        break;
    case DescriptorLifetime::transient:
        for (std::atomic_uint32_t& offset : m_per_frame_transient_allocation_offset)
        {
            offset.store(0, std::memory_order_relaxed);
        }
        break;
    default:
        LEXGINE_ASSUME;
    }
}

}