#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_ALLOCATOR_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_ALLOCATOR_H

#include <atomic>
#include <vector>

#include "engine/core/lexgine_core_fwd.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/dx/d3d12/descriptor_heap.h"

namespace lexgine::core::dx::d3d12 {

//! Base class of allocators, which reserve descriptors within a region of a descriptor heap
class DescriptorAllocator : public NamedEntity<DescriptorAllocator>
{
public:
    virtual ~DescriptorAllocator() = default;

    DescriptorHeap& descriptorHeap() const { return m_descriptor_heap; }    //! returns descriptor heap served by the allocator

    //! reserves @param capacity consecutive descriptors and returns descriptor table spanning them
    DescriptorTable allocateDescriptorTable(uint32_t capacity);

    //! creates CBV descriptors in newly reserved slots and returns GPU address of the first created descriptor
    uint64_t createConstantBufferViewDescriptors(std::vector<CBVDescriptor> const& cbv_descriptors);

    //! creates SRV descriptors in newly reserved slots and returns GPU address of the first created descriptor
    uint64_t createShaderResourceViewDescriptors(std::vector<SRVDescriptor> const& srv_descriptors);

    //! creates UAV descriptors in newly reserved slots and returns GPU address of the first created descriptor
    uint64_t createUnorderedAccessViewDescriptors(std::vector<UAVDescriptor> const& uav_descriptors);

    virtual void reset() = 0;    //! releases all descriptors reserved by the allocator

protected:
    DescriptorAllocator(DescriptorHeap& descriptor_heap);

    //! reserves @param descriptor_count consecutive descriptors and returns offset of the first one from the beginning of the heap
    virtual uint32_t reserveDescriptors(uint32_t descriptor_count) = 0;

private:
    DescriptorHeap& m_descriptor_heap;
};


//! Linearly growing allocator of descriptors living until the allocator is reset
class PersistentDescriptorAllocator final : public DescriptorAllocator
{
public:
    /*! creates allocator serving @param region_size descriptors of @param descriptor_heap
     starting at @param region_offset
    */
    PersistentDescriptorAllocator(DescriptorHeap& descriptor_heap, uint32_t region_offset, uint32_t region_size);

    void reset() override;

protected:
    uint32_t reserveDescriptors(uint32_t descriptor_count) override;

private:
    uint32_t const m_region_offset;
    uint32_t const m_region_size;
    std::atomic_uint32_t m_allocation_offset;
};


//! Allocator of descriptors living for a single frame. Keeps a separate page of descriptors for each frame in flight
class TransientDescriptorAllocator final : public DescriptorAllocator
{
public:
    /*! creates allocator serving @param descriptor_heap starting at @param region_offset. Here @param region_size
     is the descriptor capacity of a single frame: the allocator occupies region_size times the maximal number of
     frames in flight descriptors
    */
    TransientDescriptorAllocator(DescriptorHeap& descriptor_heap, uint32_t region_offset, uint32_t region_size,
        GlobalSettings const& global_settings);

    void nextFrame();    //! switches allocations to the page of the next frame and releases descriptors previously reserved in that page
    void reset() override;

protected:
    uint32_t reserveDescriptors(uint32_t descriptor_count) override;

private:
    uint32_t const m_region_offset;
    uint32_t const m_page_size;
    uint32_t const m_frames_in_flight;
    uint32_t m_active_frame_index;
    std::vector<std::atomic_uint32_t> m_per_frame_allocation_offset;
};

}

#endif
