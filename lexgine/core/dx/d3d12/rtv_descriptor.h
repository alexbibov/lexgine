#ifndef LEXGINE_CORE_DX_D3D12_RTV_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_RTV_DESCRIPTOR_H

#include <cstdint>
#include <utility>
#include <d3d12.h>

#include "lexgine_core_dx_d3d12_fwd.h"


namespace lexgine::core::dx::d3d12 {

struct RTVBufferInfo final
{
    uint64_t first_element = 0ULL;
    uint32_t num_elements;
};


struct RTVTextureInfo final
{
    uint32_t mip_level_slice = 0U;

    uint32_t _3d_texture_first_layer = 0U;    //!< first layer of 3D texture to render into (ignored for 1D and 2D textures)
    uint32_t _3d_texture_num_layers = 0xFFFFFFFF;    //! < number of layers of 3D texture to be available for rendering (ignored for 1D and 2D textures)
};


struct RTVTextureArrayInfo final
{
    uint32_t mip_level_slice = 0U;
    uint32_t first_array_element = 0U;
    uint32_t num_array_elements = 1U;
};


class RTVDescriptor final
{
public:
    RTVDescriptor(Resource const& resource, RTVBufferInfo const& buffer_info);

    RTVDescriptor(Resource const& resource, RTVTextureInfo const& texture_info);

    RTVDescriptor(Resource const& resource, RTVTextureArrayInfo const& texture_array_info);

    /*! Normally, DXGI format for the SRV descriptor is fetched from resource descriptor.
     This function can be use if another setting for the format is preferable
    */
    void overrideFormat(DXGI_FORMAT format);

    D3D12_RENDER_TARGET_VIEW_DESC nativeDescriptor() const;
    Resource const& associatedResource() const;

    uint32_t viewedMipmapLevel() const;    //! mipmap level associated with the view
    std::pair<uint32_t, uint32_t> viewedSubArray() const;    //! returns first array slice and the number of array layers associated with the view

private:
    Resource const& m_resource_ref;
    D3D12_RENDER_TARGET_VIEW_DESC m_native;
};


}


#endif