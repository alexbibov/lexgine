#ifndef LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_VIEW_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_CONSTANT_BUFFER_VIEW_DESCRIPTOR_H

#include <d3d12.h>
#include <cstdint>

#include "lexgine_core_dx_d3d12_fwd.h"

namespace lexgine::core::dx::d3d12 {

class ConstantBufferViewDescriptor final
{
public:
    ConstantBufferViewDescriptor(Resource const& resource,
        uint32_t offset_from_start, uint32_t view_size_in_bytes);

    D3D12_CONSTANT_BUFFER_VIEW_DESC nativeDescriptor() const;
    Resource const& associatedResource() const;

private:
    Resource const& m_resource_ref;
    D3D12_CONSTANT_BUFFER_VIEW_DESC m_native;
};

}

#endif
