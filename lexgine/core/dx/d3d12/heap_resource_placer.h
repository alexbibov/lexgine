#ifndef LEXGINE_CORE_DX_D3D12_HEAP_RESOURCE_PLACER_H

#include "resource.h"

#include <list>

namespace lexgine {namespace core {namespace dx {namespace d3d12 {


//! Helper class that simplifies allocation of resources within the heap
class HeapResourcePlacer final
{
public:
    HeapResourcePlacer(Heap& heap);

    HeapResourcePlacer(HeapResourcePlacer const&) = delete;
    HeapResourcePlacer(HeapResourcePlacer&&) = default;

    //! adds new resource to the heap and moves the current allocation offset caret accordingly
    Resource addResource(ResourceState const& initial_state, D3D12_CLEAR_VALUE const& optimized_clear_value, ResourceDescriptor const& descriptor);

    //! resets heap allocation caret to zero. Could be used for resource aliasing
    void reset();


private:
    Heap& m_heap;    //!< heap assigned to the allocator
    uint64_t m_current_offset;    //!< offset where to locate the next resource
};


}}}}

#define LEXGINE_CORE_DX_D3D12_HEAP_RESOURCE_PLACER_H
#endif
