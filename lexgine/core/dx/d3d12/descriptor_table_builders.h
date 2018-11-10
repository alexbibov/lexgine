#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_BUILDERS_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_BUILDERS_H

#include "lexgine_core_dx_d3d12_fwd.h"

namespace lexgine::core::dx::d3d12
{

class CBVDescriptorTable
{

};

class CBVDescriptorTableBuilder
{
public:
    CBVDescriptorTableBuilder(DescriptorHeap& m_target_descriptor_heap);

private:
    DescriptorHeap& m_target_descriptor_heap;
};

}

#endif
