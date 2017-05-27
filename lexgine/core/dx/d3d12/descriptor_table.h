#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_H

#include "entity.h"
#include "class_names.h"
#include "descriptor_heap.h"

namespace lexgine {namespace core {namespace dx {namespace d3d12 {

//! Wraps descriptor heap that can hold CBV, UAV, and SRV descriptors
class DescriptorTable_CBV_UAV_SRV final : public NamedEntity<class_names::D3D12DescriptorTable>
{
public:
    DescriptorTable_CBV_UAV_SRV(DescriptorHeap& descriptor_heap);



private:
    DescriptorHeap& m_descriptor_heap;    //!< descriptor heap housing this descriptor table

};

}}}}

#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_H
#endif
