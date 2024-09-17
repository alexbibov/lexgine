#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_ALLOCATION_MANAGER_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_ALLOCATION_MANAGER_H

#include <atomic>
#include <vector>

#include "engine/core/misc/optional.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"

#include "cbv_descriptor.h"
#include "srv_descriptor.h"
#include "uav_descriptor.h"
#include "sampler_descriptor.h"
#include "rtv_descriptor.h"
#include "dsv_descriptor.h"


namespace lexgine::core::dx::d3d12 {

class DescriptorAllocationManager
{
public:
    struct UPointer 
    {
        uint32_t descriptor_offset;
    };

public:
    DescriptorAllocationManager(DescriptorHeap& descriptor_heap, size_t capacity)
        : m_target_descriptor_heap{ descriptor_heap }
        , m_capacity{ capacity }
    {

    }


    DescriptorHeap const& heap() const { return m_target_descriptor_heap; }

    virtual uint64_t getBaseGpuAddress() const = 0;

    virtual misc::Optional<UPointer> getDescriptor(SRVDescriptor const& desc) = 0;
    virtual misc::Optional<UPointer> getDescriptor(UAVDescriptor const& desc) = 0;
    virtual misc::Optional<UPointer> getDescriptor(CBVDescriptor const& desc) = 0;

    virtual misc::Optional<UPointer> getDescriptor(SamplerDescriptor const& desc) = 0;

    virtual misc::Optional<UPointer> getDescriptor(DSVDescriptor const& desc) = 0;
    virtual misc::Optional<UPointer> getDescriptor(RTVDescriptor const& desc) = 0;


    virtual UPointer getOrCreateDescriptor(SRVDescriptor const& desc) = 0;
    virtual UPointer getOrCreateDescriptor(UAVDescriptor const& desc) = 0;
    virtual UPointer getOrCreateDescriptor(CBVDescriptor const& desc) = 0;

    virtual UPointer getOrCreateDescriptor(SamplerDescriptor const& desc) = 0;

    virtual UPointer getOrCreateDescriptor(DSVDescriptor const& desc) = 0;
    virtual UPointer getOrCreateDescriptor(RTVDescriptor const& desc) = 0;


    virtual void createDescriptors(std::vector<SRVDescriptor> const& srv_descriptors)
    {
        for (auto const& e : srv_descriptors)
        {
            getOrCreateDescriptor(e);
        }
    }

    virtual void createDescriptors(std::vector<UAVDescriptor> const& uav_descriptors)
    {
        for (auto const& e : uav_descriptors) {
            getOrCreateDescriptor(e);
        }
    }
    virtual void createDescriptors(std::vector<CBVDescriptor> const& cbv_descriptors)
    {
        for (auto const& e : cbv_descriptors) {
            getOrCreateDescriptor(e);
        }
    }

    virtual void createDescriptors(std::vector<SamplerDescriptor> const& sampler_descriptors)
    {
        for (auto const& e : sampler_descriptors) {
            getOrCreateDescriptor(e);
        }
    }

    virtual void createDescriptors(std::vector<DSVDescriptor> const& dsv_descriptors)
    {
        for (auto const& e : dsv_descriptors) {
            getOrCreateDescriptor(e);
        }
    }
    virtual void createDescriptors(std::vector<RTVDescriptor> const& rtv_descriptors)
    {
        for (auto const& e : rtv_descriptors) {
            getOrCreateDescriptor(e);
        }
    }

protected:
    size_t capacity() const { return m_capacity; }

    template<typename T>
    std::pair<UPointer, bool> createDescriptor(uint32_t offset, T const& descriptor, uint64_t(DescriptorHeap::* descriptor_creator)(size_t, T const&))
    {
        misc::Optional<UPointer> descriptor_pointer = getDescriptor(descriptor);
        if (descriptor_pointer.isValid())
            return std::make_pair(static_cast<UPointer>(descriptor_pointer), false);

        (m_target_descriptor_heap.*descriptor_creator)(offset, descriptor);
        return std::make_pair(UPointer{ offset }, true );
    }

    template<typename T>
    void createDescriptors(uint32_t offset, std::vector<T> const& descriptors, uint64_t(DescriptorHeap::* descriptor_creator)(size_t, T const&))
    {
        for (T const& e : descriptors)
        {
            createDescriptor(offset, e, descriptor_creator);
        }
    }

private:
    DescriptorHeap& m_target_descriptor_heap;
    size_t const m_capacity;
};

}

#endif