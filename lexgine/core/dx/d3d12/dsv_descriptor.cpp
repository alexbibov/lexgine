#include "dsv_descriptor.h"
#include "resource.h"

#include <cassert>

using namespace lexgine::core::dx::d3d12;


DSVDescriptor::DSVDescriptor(PlacedResource const& resource, 
    DSVTextureInfo const& texture_info, DSVFlags flags):
    m_resource_ref{ resource }
{
    auto resource_desc = resource.descriptor();

    assert(resource_desc.dimension == ResourceDimension::texture1d
        || resource_desc.dimension == ResourceDimension::texture2d);

    m_native.Format = resource_desc.format;
    m_native.Flags = static_cast<D3D12_DSV_FLAGS>(flags);

    switch (resource_desc.dimension)
    {
    case ResourceDimension::texture1d:
        m_native.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
        m_native.Texture1D.MipSlice = static_cast<UINT>(texture_info.mip_level_slice);
        break;

    case ResourceDimension::texture2d:
        m_native.ViewDimension = resource_desc.multisampling_format.count <= 1
            ? D3D12_DSV_DIMENSION_TEXTURE2D
            : D3D12_DSV_DIMENSION_TEXTURE2DMS;
        m_native.Texture2D.MipSlice = static_cast<UINT>(texture_info.mip_level_slice);
        break;
    }
}

DSVDescriptor::DSVDescriptor(PlacedResource const& resource, 
    DSVTextureArrayInfo const& texture_array_info, DSVFlags flags):
    m_resource_ref{ resource }
{
    auto resource_desc = resource.descriptor();

    assert(resource_desc.dimension == ResourceDimension::texture1d
        || resource_desc.dimension == ResourceDimension::texture2d);

    m_native.Format = resource_desc.format;
    m_native.Flags = static_cast<D3D12_DSV_FLAGS>(flags);

    switch (resource_desc.dimension)
    {
    case ResourceDimension::texture1d:
        m_native.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        m_native.Texture1DArray.FirstArraySlice = static_cast<UINT>(texture_array_info.first_array_element);
        m_native.Texture1DArray.ArraySize = static_cast<UINT>(texture_array_info.num_array_elements);
        m_native.Texture1DArray.MipSlice = static_cast<UINT>(texture_array_info.mip_level_slice);
        break;

    case ResourceDimension::texture2d:
        if (resource_desc.multisampling_format.count <= 1)
        {
            m_native.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            m_native.Texture2DArray.FirstArraySlice = static_cast<UINT>(texture_array_info.first_array_element);
            m_native.Texture2DArray.ArraySize = static_cast<UINT>(texture_array_info.num_array_elements);
            m_native.Texture2DArray.MipSlice = static_cast<UINT>(texture_array_info.mip_level_slice);
        }
        else
        {
            m_native.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            m_native.Texture2DMSArray.FirstArraySlice = static_cast<UINT>(texture_array_info.first_array_element);
            m_native.Texture2DMSArray.ArraySize = static_cast<UINT>(texture_array_info.mip_level_slice);
        }
        break;
    }
}

void DSVDescriptor::overrideFormat(DXGI_FORMAT format)
{
    m_native.Format = format;
}

D3D12_DEPTH_STENCIL_VIEW_DESC DSVDescriptor::nativeDescriptor() const
{
    return m_native;
}

PlacedResource const& DSVDescriptor::associatedResource() const
{
    return m_resource_ref;
}
