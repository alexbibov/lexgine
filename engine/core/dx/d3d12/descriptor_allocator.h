#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_ALLOCATOR_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_ALLOCATOR_H

#include <atomic>
#include <optional>
#include <vector>

#include "engine/core/lexgine_core_fwd.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/dx/d3d12/descriptor_heap.h"

namespace lexgine::core::dx::d3d12 {

enum class DescriptorLifetime
{
    persistent,
    transient 
};

class DescriptorAllocator : public ProvidesGlobals, public NamedEntity<DescriptorAllocator>
{
public:
    DescriptorAllocator(Globals& globals);
    uint64_t createConstantBufferViewDescriptors(std::vector<CBVDescriptor> const& cbv_descriptors, DescriptorLifetime lifetime);
    uint64_t createShaderResourceViewDescriptors(std::vector<SRVDescriptor> const& srv_descriptors, DescriptorLifetime lifetime);
    uint64_t createUnorderedAccessViewDescriptors(std::vector<UAVDescriptor> const& uav_descriptors, DescriptorLifetime lifetime);

    void nextFrame();
    void reset(DescriptorLifetime lifetime);

private:
    std::optional<size_t> reserveDescriptors(uint32_t descriptor_count, DescriptorLifetime lifetime);

private:
    DescriptorHeap& m_descriptor_heap;
    FrameProgressTracker const& m_frame_progress_tracker;
    uint32_t const m_frames_in_flight;
    uint32_t const m_persistent_page_size;
    uint32_t const m_transient_page_size;
    uint32_t m_active_frame_index;
    std::atomic_uint32_t m_static_allocation_offset;
    std::vector<std::atomic_uint32_t> m_per_frame_transient_allocation_offset;
};

}

#endif