#ifndef LEXGINE_CORE_DX_D3D12_DSV_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_DSV_DESCRIPTOR_H

#include <cstdint>
#include <utility>

#include <d3d12.h>

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
    DSVDescriptor(Resource const& resource, 
        DSVTextureInfo const& texture_info, 
        DSVFlags flags = DSVFlags::none);

    DSVDescriptor(Resource const& resource, 
        DSVTextureArrayInfo const& texture_array_info,
        DSVFlags flags = DSVFlags::none);

    /*! Normally, DXGI format for the SRV descriptor is fetched from resource descriptor.
     This function can be useful if another setting for the format is preferable
    */
    void overrideFormat(DXGI_FORMAT format);

    D3D12_DEPTH_STENCIL_VIEW_DESC nativeDescriptor() const;
    Resource const& associatedResource() const;

    uint32_t mipmapLevel() const;    //! returns mipmap level attached to the view

    //! for compatible resources returns the first array slice and the total number of array elements that were attached to the view
    std::pair<uint64_t, uint32_t> arrayOffsetAndSize() const;

private:
    Resource const& m_resource_ref;
    D3D12_DEPTH_STENCIL_VIEW_DESC m_native;
};

}

#endif
