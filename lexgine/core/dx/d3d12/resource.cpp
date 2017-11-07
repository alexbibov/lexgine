#include "resource.h"
#include "device.h"

using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core;

uint64_t ResourceDescriptor::getAllocationSize(Device const& device, uint32_t node_exposure_mask) const
{
    D3D12_RESOURCE_DESC desc;
    desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(dimension);
    desc.Alignment = static_cast<UINT64>(alignment);
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = depth;
    desc.MipLevels = num_mipmaps;
    desc.Format = format;
    desc.SampleDesc.Count = multisampling_format.count;
    desc.SampleDesc.Quality = multisampling_format.quality;
    desc.Layout = static_cast<D3D12_TEXTURE_LAYOUT>(layout);
    desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(flags.getValue());

    return device.native()->GetResourceAllocationInfo(node_exposure_mask, 1, &desc).SizeInBytes;
}

ResourceDescriptor ResourceDescriptor::CreateBuffer(uint64_t size, ResourceFlags flags)
{
    ResourceDescriptor desc;
    desc.dimension = ResourceDimension::buffer;
    desc.alignment = ResourceAlignment::_64kb;
    desc.width = size;
    desc.height = 1;
    desc.depth = 1;
    desc.num_mipmaps = 1;
    desc.format = DXGI_FORMAT_UNKNOWN;
    desc.multisampling_format.count = 1;
    desc.multisampling_format.quality = 0;
    desc.layout = TextureLayout::row_major;
    desc.flags = flags;

    return desc;
}


ResourceDescriptor ResourceDescriptor::CreateTexture1D(uint64_t width, uint16_t array_size, DXGI_FORMAT format, uint16_t num_mipmaps, ResourceFlags flags,
    MultiSamplingFormat ms_format, ResourceAlignment alignment, TextureLayout layout)
{
    ResourceDescriptor desc;
    desc.dimension = ResourceDimension::texture1d;
    desc.alignment = alignment;
    desc.width = width;
    desc.height = 1;
    desc.depth = array_size;
    desc.num_mipmaps = num_mipmaps;
    desc.format = format;
    desc.multisampling_format = ms_format;
    desc.layout = layout;
    desc.flags = flags;

    return desc;
}

ResourceDescriptor ResourceDescriptor::CreateTexture2D(uint64_t width, uint32_t height, uint16_t array_size, DXGI_FORMAT format, uint16_t num_mipmaps,
    ResourceFlags flags, MultiSamplingFormat ms_format, ResourceAlignment alignment, TextureLayout layout)
{
    ResourceDescriptor desc;
    desc.dimension = ResourceDimension::texture2d;
    desc.alignment = alignment;
    desc.width = width;
    desc.height = height;
    desc.depth = array_size;
    desc.num_mipmaps = num_mipmaps;
    desc.format = format;
    desc.multisampling_format = ms_format;
    desc.layout = layout;
    desc.flags = flags;

    return desc;
}

ResourceDescriptor ResourceDescriptor::CreateTexture3D(uint64_t width, uint32_t height, uint16_t depth, DXGI_FORMAT format, uint16_t num_mipmaps, ResourceFlags flags, MultiSamplingFormat ms_format, ResourceAlignment alignment, TextureLayout layout)
{
    ResourceDescriptor desc;
    desc.dimension = ResourceDimension::texture3d;
    desc.alignment = alignment;
    desc.width = width;
    desc.height = height;
    desc.depth = depth;
    desc.num_mipmaps = num_mipmaps;
    desc.format = format;
    desc.multisampling_format = ms_format;
    desc.layout = layout;
    desc.flags = flags;

    return desc;
}

Resource::Resource(Heap & heap, uint64_t heap_offset, ResourceState const & initial_state, D3D12_CLEAR_VALUE const & optimized_clear_value, ResourceDescriptor const & descriptor):
    m_heap{ heap },
    m_offset{ heap_offset },
    m_state{ initial_state }
{
    D3D12_RESOURCE_DESC desc;
    desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(descriptor.dimension);
    desc.Alignment = static_cast<UINT64>(descriptor.alignment);
    desc.Width = descriptor.width;
    desc.Height = descriptor.height;
    desc.DepthOrArraySize = descriptor.depth;
    desc.MipLevels = descriptor.num_mipmaps;
    desc.Format = descriptor.format;
    desc.SampleDesc.Count = descriptor.multisampling_format.count;
    desc.SampleDesc.Quality = descriptor.multisampling_format.quality;
    desc.Layout = static_cast<D3D12_TEXTURE_LAYOUT>(descriptor.layout);
    desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(descriptor.flags.getValue());

    LEXGINE_LOG_ERROR_IF_FAILED(this,
        heap.device().native()->CreatePlacedResource(heap.native().Get(), heap_offset, &desc, static_cast<D3D12_RESOURCE_STATES>(initial_state.getValue()), &optimized_clear_value, IID_PPV_ARGS(&m_resource)),
        S_OK);
}

Heap& Resource::heap() const
{
    return m_heap;
}

uint64_t Resource::offset() const
{
    return m_offset;
}

ComPtr<ID3D12Resource> Resource::native() const
{
    return m_resource;
}

ResourceState Resource::getCurrentState() const
{
    return m_state;
}
