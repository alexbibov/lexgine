#ifndef LEXGINE_CORE_DX_D3D12_DSV_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_DSV_DESCRIPTOR_H

#include <d3d12.h>
#include <cstdint>

#include "lexgine_core_dx_d3d12_fwd.h"


namespace lexgine::core::dx::d3d12 {

enum class DSVFlags
{
    none, read_only_depth, read_only_stencil
};

struct DSVTextureInfo final
{
    uint32_t mip_level_slice = 0U;
};


struct DSVTextureArrayInfo final
{
    uint32_t mip_level_slice = 0U;
    uint32_t first_array_element = 0U;
    uint32_t num_array_elements = 1U;
};


class DSVDescriptor final
{
public:
    DSVDescriptor(PlacedResource const& resource, 
        DSVTextureInfo const& texture_info, 
        DSVFlags flags = DSVFlags::none);

    DSVDescriptor(PlacedResource const& resource, 
        DSVTextureArrayInfo const& texture_array_info,
        DSVFlags flags = DSVFlags::none);

    /*! Normally, DXGI format for the SRV descriptor is fetched from resource descriptor.
     This function can be useful if another setting for the format is preferable
    */
    void overrideFormat(DXGI_FORMAT format);

    D3D12_DEPTH_STENCIL_VIEW_DESC nativeDescriptor() const;
    PlacedResource const& associatedResource() const;

private:
    PlacedResource const& m_resource_ref;
    D3D12_DEPTH_STENCIL_VIEW_DESC m_native;
};

}

#endif
