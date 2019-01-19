#ifndef LEXGINE_CORE_DX_D3D12_CBV_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_CBV_DESCRIPTOR_H

#include <cstdint>

#include <d3d12.h>

#include "lexgine_core_dx_d3d12_fwd.h"

namespace lexgine::core::dx::d3d12 {

class CBVDescriptor final
{
public:
    CBVDescriptor(Resource const& resource,
        uint32_t offset_from_start, uint32_t view_size_in_bytes);

    D3D12_CONSTANT_BUFFER_VIEW_DESC nativeDescriptor() const;
    Resource const& associatedResource() const;

    uint64_t gpuVirtualAddress() const;
    uint32_t size() const;

private:
    Resource const& m_resource_ref;
    D3D12_CONSTANT_BUFFER_VIEW_DESC m_native;
};

}

#endif
