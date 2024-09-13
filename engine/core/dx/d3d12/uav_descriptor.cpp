#include <cassert>

#include "uav_descriptor.h"
#include "resource.h"


using namespace lexgine::core::dx::d3d12;

UAVDescriptor::UAVDescriptor(Resource const& resource, 
    UAVBufferInfo const& buffer_info,
    Resource const* p_counter_resource)
    : HashableDescriptor{ resource, p_counter_resource, m_native }
    , m_resource_ref{ resource }
    , m_counter_resource_ptr{ p_counter_resource }
{
    auto resource_desc = resource.descriptor();

    assert(resource_desc.dimension == ResourceDimension::buffer);

    m_native.Format = m_counter_resource_ptr ? DXGI_FORMAT_UNKNOWN : resource_desc.format;
    m_native.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    m_native.Buffer.FirstElement = static_cast<UINT64>(buffer_info.first_element);
    m_native.Buffer.NumElements = static_cast<UINT>(buffer_info.num_elements);
    m_native.Buffer.StructureByteStride = static_cast<UINT>(buffer_info.structure_byte_stride);
    m_native.Buffer.CounterOffsetInBytes = static_cast<UINT64>(buffer_info.counter_offset_in_bytes);
    m_native.Buffer.Flags = static_cast<D3D12_BUFFER_UAV_FLAGS>(buffer_info.flags);
}

UAVDescriptor::UAVDescriptor(Resource const& resource, 
    UAVTextureInfo const& texture_info,
    Resource const* p_counter_resource)
    : HashableDescriptor{ resource, p_counter_resource, m_native }
    , m_resource_ref{ resource }
    , m_counter_resource_ptr{ p_counter_resource }
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

UAVDescriptor::UAVDescriptor(Resource const& resource, 
    UAVTextureArrayInfo const& texture_array_info,
    Resource const* p_counter_resource)
    : HashableDescriptor{ resource, p_counter_resource, m_native }
    , m_resource_ref{ resource }
    , m_counter_resource_ptr{ p_counter_resource }
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

Resource const& UAVDescriptor::associatedResource() const
{
    return m_resource_ref;
}

Resource const* UAVDescriptor::associatedCounterResourcePtr() const
{
    return m_counter_resource_ptr;
}

uint32_t UAVDescriptor::mipmapLevel() const
{
    switch (m_native.ViewDimension)
    {
    case D3D12_UAV_DIMENSION_BUFFER:
        return 0U;

    case D3D12_UAV_DIMENSION_TEXTURE1D:
    case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
    case D3D12_UAV_DIMENSION_TEXTURE2D:
    case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
    case D3D12_UAV_DIMENSION_TEXTURE3D:
        return static_cast<uint32_t>(m_native.Texture1D.MipSlice);
    }

    return static_cast<uint32_t>(-1);
}

std::pair<uint64_t, uint32_t> UAVDescriptor::arrayOffsetAndSize() const
{
    switch (m_native.ViewDimension)
    {
    case D3D12_UAV_DIMENSION_BUFFER:
        return std::make_pair(m_native.Buffer.FirstElement,
            static_cast<uint32_t>(m_native.Buffer.NumElements));

    case D3D12_UAV_DIMENSION_TEXTURE1D:
    case D3D12_UAV_DIMENSION_TEXTURE2D:
        return std::make_pair(0U, 1U);

    case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
    case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
    case D3D12_UAV_DIMENSION_TEXTURE3D:
        return std::make_pair(static_cast<uint32_t>(m_native.Texture1DArray.FirstArraySlice),
            static_cast<uint32_t>(m_native.Texture1DArray.ArraySize));
    }

    return std::make_pair(static_cast<uint64_t>(-1), static_cast<uint32_t>(-1));
}
