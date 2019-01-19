#include <cassert>

#include "rtv_descriptor.h"
#include "resource.h"

using namespace lexgine::core::dx::d3d12;

RTVDescriptor::RTVDescriptor(Resource const& resource, RTVBufferInfo const& buffer_info):
    m_resource_ref{ resource }
{
    auto resource_desc = resource.descriptor();

    assert(resource_desc.dimension == ResourceDimension::buffer);

    m_native.Format = resource_desc.format;
    m_native.ViewDimension = D3D12_RTV_DIMENSION_BUFFER;
    m_native.Buffer.FirstElement = static_cast<UINT64>(buffer_info.first_element);
    m_native.Buffer.NumElements = static_cast<UINT>(buffer_info.num_elements);
}

RTVDescriptor::RTVDescriptor(Resource const& resource, RTVTextureInfo const& texture_info):
    m_resource_ref{ resource }
{
    auto resource_desc = resource.descriptor();

    assert(resource_desc.dimension == ResourceDimension::texture1d
        || resource_desc.dimension == ResourceDimension::texture2d
        || resource_desc.dimension == ResourceDimension::texture3d);


    m_native.Format = resource_desc.format;

    switch (resource_desc.dimension)
    {
    case ResourceDimension::texture1d:
        m_native.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
        m_native.Texture1D.MipSlice = static_cast<UINT>(texture_info.mip_level_slice);
        break;

    case ResourceDimension::texture2d:
        m_native.ViewDimension = resource_desc.multisampling_format.count <= 1
            ? D3D12_RTV_DIMENSION_TEXTURE2D
            : D3D12_RTV_DIMENSION_TEXTURE2DMS;
        m_native.Texture2D.MipSlice = static_cast<UINT>(texture_info.mip_level_slice);
        m_native.Texture2D.PlaneSlice = 0;
        break;

    case ResourceDimension::texture3d:
        m_native.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        m_native.Texture3D.MipSlice = static_cast<UINT>(texture_info.mip_level_slice);
        m_native.Texture3D.FirstWSlice = static_cast<UINT>(texture_info._3d_texture_first_layer);
        m_native.Texture3D.WSize = static_cast<UINT>(texture_info._3d_texture_num_layers);
        break;
    }
}

RTVDescriptor::RTVDescriptor(Resource const& resource, RTVTextureArrayInfo const& texture_array_info):
    m_resource_ref{ resource }
{
    auto resource_desc = resource.descriptor();

    assert((resource_desc.dimension == ResourceDimension::texture1d
        || resource_desc.dimension == ResourceDimension::texture2d)
        && resource_desc.depth >=
        texture_array_info.first_array_element + texture_array_info.num_array_elements);

    switch (resource_desc.dimension)
    {
    case ResourceDimension::texture1d:
        m_native.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        m_native.Texture1DArray.FirstArraySlice = static_cast<UINT>(texture_array_info.first_array_element);
        m_native.Texture1DArray.ArraySize = static_cast<UINT>(texture_array_info.num_array_elements);
        m_native.Texture1DArray.MipSlice = static_cast<UINT>(texture_array_info.mip_level_slice);
        break;

    case ResourceDimension::texture2d:
        if (resource_desc.multisampling_format.count <= 1)
        {
            // normal 2d texture

            m_native.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            m_native.Texture2DArray.FirstArraySlice = static_cast<UINT>(texture_array_info.first_array_element);
            m_native.Texture2DArray.ArraySize = static_cast<UINT>(texture_array_info.num_array_elements);
            m_native.Texture2DArray.MipSlice = static_cast<UINT>(texture_array_info.mip_level_slice);
            m_native.Texture2DArray.PlaneSlice = 0;
        }
        else
        {
            // multi-sampled 2d texture

            m_native.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            m_native.Texture2DMSArray.FirstArraySlice = static_cast<UINT>(texture_array_info.first_array_element);
            m_native.Texture2DMSArray.ArraySize = static_cast<UINT>(texture_array_info.num_array_elements);
        }
        
        break;
    }
}

void RTVDescriptor::overrideFormat(DXGI_FORMAT format)
{
    m_native.Format = format;
}

D3D12_RENDER_TARGET_VIEW_DESC RTVDescriptor::nativeDescriptor() const
{
    return m_native;
}

Resource const& RTVDescriptor::associatedResource() const
{
    return m_resource_ref;
}

uint32_t RTVDescriptor::mipmapLevel() const
{
    switch (m_native.ViewDimension)
    {
    
    case D3D12_RTV_DIMENSION_TEXTURE1D:
    case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
    case D3D12_RTV_DIMENSION_TEXTURE2D:
    case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
    case D3D12_RTV_DIMENSION_TEXTURE3D:
        return static_cast<uint32_t>(m_native.Texture1D.MipSlice);

    case D3D12_RTV_DIMENSION_BUFFER:
    case D3D12_RTV_DIMENSION_TEXTURE2DMS:
    case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
        return 0;
    }

    return static_cast<uint32_t>(-1);
}

std::pair<uint64_t, uint32_t> RTVDescriptor::arrayOffsetAndSize() const
{
    switch (m_native.ViewDimension)
    {
    case D3D12_RTV_DIMENSION_TEXTURE1D:
    case D3D12_RTV_DIMENSION_TEXTURE2D:
    case D3D12_RTV_DIMENSION_TEXTURE2DMS:
        return std::make_pair(0U, 1U);

    case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
    case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
    case D3D12_RTV_DIMENSION_TEXTURE3D:
        return std::make_pair(m_native.Texture1DArray.FirstArraySlice,
            static_cast<uint32_t>(m_native.Texture1DArray.ArraySize));

    case D3D12_RTV_DIMENSION_BUFFER:
        return std::make_pair(m_native.Buffer.FirstElement, m_native.Buffer.NumElements);
        

    case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
        return std::make_pair(static_cast<uint32_t>(m_native.Texture2DMSArray.FirstArraySlice),
            static_cast<uint32_t>(m_native.Texture2DMSArray.ArraySize));
    }

    return std::make_pair(static_cast<uint64_t>(-1), static_cast<uint32_t>(-1));
}
