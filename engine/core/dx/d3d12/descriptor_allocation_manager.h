#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_ALLOCATION_MANAGER_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_ALLOCATION_MANAGER_H

#include <atomic>
#include <vector>
#include <type_traits>

#include "lexgine_core_dx_d3d12_fwd.h"

#include "descriptor_heap.h"
#include "cbv_descriptor.h"
#include "srv_descriptor.h"
#include "uav_descriptor.h"
#include "sampler_descriptor.h"
#include "rtv_descriptor.h"
#include "dsv_descriptor.h"


namespace lexgine::core::dx::d3d12 {

class DescriptorAllocationManager : public NamedEntity<class_names::D3D12_DescriptorHeapAllocationManager>
{
public:
    static constexpr size_t INVALID_POINTER = static_cast<size_t>(-1);

public:
    explicit DescriptorAllocationManager(DescriptorHeap& descriptor_heap)
        : m_descriptor_table{ 
            .offset = INVALID_POINTER,
            .cpu_pointer = INVALID_POINTER,
            .gpu_pointer = INVALID_POINTER,
            .descriptor_count = 0,
            .descriptor_size = 0,
            .p_heap = &descriptor_heap 
        }
    {

    }

    bool build(size_t allocator_capacity)
    {
        if (m_descriptor_table.offset != INVALID_POINTER)
            return allocator_capacity <= m_descriptor_table.descriptor_count;
        DescriptorHeap* p_heap = m_descriptor_table.p_heap;
        m_descriptor_table = p_heap->allocateDescriptorTable(allocator_capacity);
        return true;
    }

    DescriptorTable const& getDescriptorTable() const { return m_descriptor_table; }

    virtual size_t getOrCreateDescriptor(size_t offset_hint, SRVDescriptor const& desc)
    {
        return createDescriptor(offset_hint, desc, &DescriptorHeap::createShaderResourceViewDescriptor);
    }

    virtual size_t getOrCreateDescriptor(size_t offset_hint, UAVDescriptor const& desc)
    {
        return createDescriptor(offset_hint, desc, &DescriptorHeap::createUnorderedAccessViewDescriptor);
    }

    virtual size_t getOrCreateDescriptor(size_t offset_hint, CBVDescriptor const& desc)
    {
        return createDescriptor(offset_hint, desc, &DescriptorHeap::createConstantBufferViewDescriptor);
    }

    virtual size_t getOrCreateDescriptor(size_t offset_hint, SamplerDescriptor const& desc)
    {
        return createDescriptor(offset_hint, desc, &DescriptorHeap::createSamplerDescriptor);
    }

protected:
    template<typename T>
    size_t createDescriptor(size_t offset_hint, T const& descriptor, uint64_t(DescriptorHeap::* descriptor_creator)(size_t, T const&))
    {
        (m_descriptor_table.p_heap->*descriptor_creator)(m_descriptor_table.offset + offset_hint, descriptor);
        return offset_hint;
    }

private:
    DescriptorTable m_descriptor_table;
};

}

#endif