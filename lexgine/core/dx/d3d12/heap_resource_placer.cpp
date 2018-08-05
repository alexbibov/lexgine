#include "heap_resource_placer.h"
#include "device.h"

#include <cassert>

using namespace lexgine::core::dx::d3d12;


HeapResourcePlacer::HeapResourcePlacer(Heap& heap) :
    m_heap{ heap },
    m_current_offset{ 0U }
{

}

Resource HeapResourcePlacer::addResource(ResourceState const& initial_state, D3D12_CLEAR_VALUE const& optimized_clear_value, ResourceDescriptor const& descriptor)
{
    uint64_t resource_size = descriptor.getAllocationSize(m_heap.device(), m_heap.getExposureMask());
    assert(resource_size <= m_heap.capacity() - m_current_offset);

    Resource rv{ m_heap, m_current_offset, initial_state, optimized_clear_value, descriptor };
    m_current_offset += resource_size;

    return rv;
}

void HeapResourcePlacer::reset()
{
    m_current_offset = 0;
}


