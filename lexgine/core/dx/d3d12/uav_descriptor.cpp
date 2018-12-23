#include "uav_descriptor.h"
#include "resource.h"

#include <cassert>

using namespace lexgine::core::dx::d3d12;

UAVDescriptor::UAVDescriptor(PlacedResource const& resource, 
    UAVBufferInfo const& buffer_info,
    PlacedResource const* p_counter_resource):
    m_resource_ref{ resource },
    m_counter_resource_ptr{ p_counter_resource }
{
    auto resource_desc = resource.descriptor();

    assert(resource_desc.dimension == ResourceDimension::buffer);

    m_native.Format = resource_desc.format;
    m_native.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    m_native.Buffer.FirstElement = static_cast<UINT64>(buffer_info.first_element);
    m_native.Buffer.NumElements = static_cast<UINT>(buffer_info.num_elements);
    m_native.Buffer.StructureByteStride = static_cast<UINT>(buffer_info.structure_byte_stride);
    m_native.Buffer.CounterOffsetInBytes = static_cast<UINT64>(buffer_info.counter_offset_in_bytes);
    m_native.Buffer.Flags = static_cast<D3D12_BUFFER_UAV_FLAGS>(buffer_info.flags);
}

UAVDescriptor::UAVDescriptor(PlacedResource const& resource, 
    UAVTextureInfo const& texture_info,
    PlacedResource const* p_counter_resource):
    m_resource_ref{ resource },
    m_counter_resource_ptr{ p_counter_resource }
{
    auto resource_desc = resource.descriptor();

    assert((resource_desc.dimension == ResourceDimension::texture1d
        || resource_desc.dimension == ResourceDimension::texture2d
        || resource_desc.dimension == ResourceDimension::texture3d)
        && resource_desc.multisampling_format.count <= 1);

    m_native.Format = resource_desc.format;

    switch (resource_desc.dimension)
    {
    case ResourceDimension::texture1d:
        m_native.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
        m_native.Texture1D.MipSlice = static_cast<UINT>(texture_info.mip_level_slice);
        break;

    case ResourceDimension::texture2d:
        m_native.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        m_native.Texture2D.MipSlice = static_cast<UINT>(texture_info.mip_level_slice);
        m_native.Texture2D.PlaneSlice = 0;
        break;

    case ResourceDimension::texture3d:
        m_native.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        m_native.Texture3D.MipSlice = static_cast<UINT>(texture_info.mip_level_slice);
        m_native.Texture3D.FirstWSlice = static_cast<UINT>(texture_info._3d_texture_first_layer);
        m_native.Texture3D.WSize = static_cast<UINT>(texture_info._3d_texture_num_layers);
        break;
    }
}

UAVDescriptor::UAVDescriptor(PlacedResource const& resource, 
    UAVTextureArrayInfo const& texture_array_info,
    PlacedResource const* p_counter_resource):
    m_resource_ref{ resource },
    m_counter_resource_ptr{ p_counter_resource }
{
    auto resource_desc = resource.descriptor();

    assert((resource_desc.dimension == ResourceDimension::texture1d
        || resource_desc.dimension == ResourceDimension::texture2d)
        && resource_desc.multisampling_format.count <= 1
        && resource_desc.depth >= 
        texture_array_info.first_array_element + texture_array_info.num_array_elements);

    m_native.Format = resource_desc.format;

    switch (resource_desc.dimension)
    {
    case ResourceDimension::texture1d:
        m_native.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
        m_native.Texture1DArray.MipSlice = static_cast<UINT>(texture_array_info.mip_level_slice);
        m_native.Texture1DArray.FirstArraySlice = static_cast<UINT>(texture_array_info.first_array_element);
        m_native.Texture1DArray.ArraySize = static_cast<UINT>(texture_array_info.num_array_elements);
        break;

    case ResourceDimension::texture2d:
        m_native.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        m_native.Texture2DArray.MipSlice = static_cast<UINT>(texture_array_info.mip_level_slice);
        m_native.Texture2DArray.FirstArraySlice = static_cast<UINT>(texture_array_info.first_array_element);
        m_native.Texture2DArray.ArraySize = static_cast<UINT>(texture_array_info.num_array_elements);
        m_native.Texture2DArray.PlaneSlice = 0;
        break;
    }
}

void UAVDescriptor::overrideFormat(DXGI_FORMAT format)
{
    m_native.Format = format;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDescriptor::nativeDescriptor() const
{
    return m_native;
}

PlacedResource const& UAVDescriptor::associatedResource() const
{
    return m_resource_ref;
}

PlacedResource const* UAVDescriptor::associatedCounterResourcePtr() const
{
    return m_counter_resource_ptr;
}
