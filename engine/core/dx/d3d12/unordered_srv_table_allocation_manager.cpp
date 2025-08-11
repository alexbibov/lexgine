#include <cassert>
#include <thread>

#include "descriptor_heap.h"

#include "unordered_srv_table_allocation_manager.h"

namespace lexgine::core::dx::d3d12 {

UnorderedSRVTableAllocationManager::UnorderedSRVTableAllocationManager(DescriptorHeap& descriptor_heap)
    : DescriptorAllocationManager{ descriptor_heap }
{
    
}

size_t UnorderedSRVTableAllocationManager::getOrCreateDescriptor(size_t offset_hint, SRVDescriptor const& desc)
{
    if (!m_srv_lut.count(desc))
    {
        m_srv_lut[desc] = createDescriptor(offset_hint, desc, &DescriptorHeap::createShaderResourceViewDescriptor);
    }

    return m_srv_lut[desc];
}

}