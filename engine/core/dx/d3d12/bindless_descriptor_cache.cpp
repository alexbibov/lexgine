#include "descriptor_allocator.h"
#include "descriptor_heap.h"

#include "bindless_descriptor_cache.h"


namespace lexgine::core::dx::d3d12 {

BindlessDescriptorCache::BindlessDescriptorCache(DescriptorAllocator& allocator)
    : m_allocator{ allocator }
{

}

uint32_t BindlessDescriptorCache::getOrCreate(SRVDescriptor const& descriptor)
{
    std::lock_guard lock{ m_mutex };
    if (auto p = m_descriptor_slots.find(descriptor); p != m_descriptor_slots.end())
    {
        return p->second;
    }

    DescriptorTable slot = m_allocator.allocateDescriptorTable(1);
    m_allocator.descriptorHeap().createShaderResourceViewDescriptor(slot.offset, descriptor);
    uint32_t slot_index = static_cast<uint32_t>(slot.offset);
    m_descriptor_slots.emplace(descriptor, slot_index);
    return slot_index;
}

}
