#include "shader_resource_view_descriptor.h"
#include "resource.h"

#include <cassert>

using namespace lexgine::core::dx::d3d12;


ShaderResourceViewDescriptor::ShaderResourceViewDescriptor(Resource const& resource,
    ShaderResourceViewBufferInfo const& buffer_info):
    m_resource_ref{ resource }
{
    auto resource_desc = resource.descriptor();
    
    assert(resource_desc.dimension == ResourceDimension::buffer);

    m_native.Format = resource_desc.format;
    m_native.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    m_native.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    m_native.Buffer.FirstElement = static_cast<UINT64>(buffer_info.first_element);
    m_native.Buffer.NumElements = static_cast<UINT>(buffer_info.num_elements);
    m_native.Buffer.StructureByteStride = static_cast<UINT>(buffer_info.structure_byte_stride);
    m_native.Buffer.Flags = static_cast<D3D12_BUFFER_SRV_FLAGS>(buffer_info.flags);
}

ShaderResourceViewDescriptor::ShaderResourceViewDescriptor(Resource const& resource,
    ShaderResourceViewTextureInfo const& texture_info, bool is_cubemap):
    m_resource_ref{ resource }
{
    auto resource_desc = resource.descriptor();

    assert(resource_desc.dimension == ResourceDimension::texture1d
        || resource_desc.dimension == ResourceDimension::texture2d
        || resource_desc.dimension == ResourceDimension::texture3d);

    m_native.Format = resource_desc.format;
    m_native.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    switch (resource_desc.dimension)
    {
    case ResourceDimension::texture1d:
        m_native.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        m_native.Texture1D.MipLevels = static_cast<UINT>(texture_info.num_mipmap_levels);
        m_native.Texture1D.MostDetailedMip = static_cast<UINT>(texture_info.most_detailed_mipmap_level);
        m_native.Texture1D.ResourceMinLODClamp = static_cast<FLOAT>(texture_info.resource_min_lod_clamp);
        break;

    case ResourceDimension::texture2d:
        if (is_cubemap)
        {
            assert(resource_desc.depth == 6);

            m_native.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            m_native.TextureCube.MipLevels = static_cast<UINT>(texture_info.num_mipmap_levels);
            m_native.TextureCube.MostDetailedMip = static_cast<UINT>(texture_info.most_detailed_mipmap_level);
            m_native.TextureCube.ResourceMinLODClamp = static_cast<FLOAT>(texture_info.resource_min_lod_clamp);
        }
        else
        {
            m_native.ViewDimension = resource_desc.multisampling_format.count <= 1
                ? D3D12_SRV_DIMENSION_TEXTURE2D
                : D3D12_SRV_DIMENSION_TEXTURE2DMS;
            m_native.Texture2D.PlaneSlice = 0;
            m_native.Texture2D.MipLevels = static_cast<UINT>(texture_info.num_mipmap_levels);
            m_native.Texture2D.MostDetailedMip = static_cast<UINT>(texture_info.most_detailed_mipmap_level);
            m_native.Texture2D.ResourceMinLODClamp = static_cast<FLOAT>(texture_info.resource_min_lod_clamp);
        }
        
        break;

    case ResourceDimension::texture3d:
        m_native.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        m_native.Texture3D.MipLevels = static_cast<UINT>(texture_info.num_mipmap_levels);
        m_native.Texture3D.MostDetailedMip = static_cast<UINT>(texture_info.most_detailed_mipmap_level);
        m_native.Texture3D.ResourceMinLODClamp = static_cast<FLOAT>(texture_info.resource_min_lod_clamp);
        break;
    }
}

ShaderResourceViewDescriptor::ShaderResourceViewDescriptor(Resource const& resource,
    ShaderResourceViewTextureArrayInfo const& texture_array_info, bool is_cubemap):
    m_resource_ref{ resource }
{
    auto resource_desc = resource.descriptor();

    assert(resource_desc.dimension == ResourceDimension::texture1d
        || resource_desc.dimension == ResourceDimension::texture2d);

    m_native.Format = resource_desc.format;
    m_native.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;


    switch (resource_desc.dimension)
    {
    case ResourceDimension::texture1d:
        assert(resource_desc.depth >=
            texture_array_info.first_array_element + texture_array_info.num_array_elements);

        m_native.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        m_native.Texture1DArray.FirstArraySlice = static_cast<UINT>(texture_array_info.first_array_element);
        m_native.Texture1DArray.ArraySize = static_cast<UINT>(texture_array_info.num_array_elements);
        m_native.Texture1DArray.MipLevels = static_cast<UINT>(texture_array_info.num_mipmap_levels);
        m_native.Texture1DArray.MostDetailedMip = static_cast<UINT>(texture_array_info.most_detailed_mipmap_level);
        m_native.Texture1DArray.ResourceMinLODClamp = static_cast<FLOAT>(texture_array_info.resource_min_lod_clamp);
        break;

    case ResourceDimension::texture2d:
        if (is_cubemap)
        {
            // cubemap array

            assert(resource_desc.depth > 0 && resource_desc.depth % 6 == 0
                && resource_desc.depth >=
                texture_array_info.first_array_element + 6 * texture_array_info.num_array_elements);

            m_native.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            m_native.TextureCubeArray.First2DArrayFace = static_cast<UINT>(texture_array_info.first_array_element);
            m_native.TextureCubeArray.NumCubes = static_cast<UINT>(texture_array_info.num_array_elements);
            m_native.TextureCubeArray.MipLevels = static_cast<UINT>(texture_array_info.num_mipmap_levels);
            m_native.TextureCubeArray.MostDetailedMip = static_cast<UINT>(texture_array_info.most_detailed_mipmap_level);
            m_native.TextureCubeArray.ResourceMinLODClamp = static_cast<FLOAT>(texture_array_info.resource_min_lod_clamp);
        }
        else if (resource_desc.multisampling_format.count <= 1)
        {
            // simple 2d texture

            assert(resource_desc.depth >=
                texture_array_info.first_array_element + texture_array_info.num_array_elements);

            m_native.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            m_native.Texture2DArray.PlaneSlice = 0;
            m_native.Texture2DArray.FirstArraySlice = static_cast<UINT>(texture_array_info.first_array_element);
            m_native.Texture2DArray.ArraySize = static_cast<UINT>(texture_array_info.num_array_elements);
            m_native.Texture2DArray.MipLevels = static_cast<UINT>(texture_array_info.num_mipmap_levels);
            m_native.Texture2DArray.MostDetailedMip = static_cast<UINT>(texture_array_info.most_detailed_mipmap_level);
            m_native.Texture2DArray.ResourceMinLODClamp = static_cast<FLOAT>(texture_array_info.resource_min_lod_clamp);
        }
        else
        {
            // multi-sampled 2d texture

            assert(resource_desc.depth >=
                texture_array_info.first_array_element + texture_array_info.num_array_elements);

            m_native.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            m_native.Texture2DMSArray.FirstArraySlice = static_cast<UINT>(texture_array_info.first_array_element);
            m_native.Texture2DMSArray.ArraySize = static_cast<UINT>(texture_array_info.num_array_elements);
        }

        break;
    }
}

void ShaderResourceViewDescriptor::overrideFormat(DXGI_FORMAT format)
{
    m_native.Format = format;
}

D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDescriptor::nativeDescriptor() const
{
    return m_native;
}

Resource const& ShaderResourceViewDescriptor::associatedResource() const
{
    return m_resource_ref;
}

