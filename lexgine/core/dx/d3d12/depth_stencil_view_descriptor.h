#ifndef LEXGINE_CORE_DX_D3D12_DEPTH_STENCIL_VIEW_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_DEPTH_STENCIL_VIEW_DESCRIPTOR_H

#include <d3d12.h>
#include <cstdint>

#include "lexgine_core_dx_d3d12_fwd.h"


namespace lexgine::core::dx::d3d12 {

enum class DepthStencilViewFlags
{
    none, read_only_depth, read_only_stencil
};

struct DepthStencilViewTextureInfo final
{
    uint32_t mip_level_slice = 0U;
};


struct DepthStencilViewTextureArrayInfo final
{
    uint32_t mip_level_slice = 0U;
    uint32_t first_array_element = 0U;
    uint32_t num_array_elements = 1U;
};


class DepthStencilViewDescriptor final
{
public:
    DepthStencilViewDescriptor(Resource const& resource, 
        DepthStencilViewTextureInfo const& texture_info, 
        DepthStencilViewFlags flags = DepthStencilViewFlags::none);

    DepthStencilViewDescriptor(Resource const& resource, 
        DepthStencilViewTextureArrayInfo const& texture_array_info,
        DepthStencilViewFlags flags = DepthStencilViewFlags::none);

    /*! Normally, DXGI format for the SRV descriptor is fetched from resource descriptor.
     This function can be useful if another setting for the format is preferable
    */
    void overrideFormat(DXGI_FORMAT format);

    D3D12_DEPTH_STENCIL_VIEW_DESC nativeDescriptor() const;
    Resource const& associatedResource() const;

private:
    Resource const& m_resource_ref;
    D3D12_DEPTH_STENCIL_VIEW_DESC m_native;
};

}

#endif
