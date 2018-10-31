#include "resource.h"
#include "device.h"
#include "lexgine/core/exception.h"

#include <algorithm>

using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core;

uint64_t ResourceDescriptor::getAllocationSize(Device const& device, uint32_t node_exposure_mask) const
{
    D3D12_RESOURCE_DESC desc = native();
    return device.native()->GetResourceAllocationInfo(node_exposure_mask, 1, &desc).SizeInBytes;
}

D3D12_RESOURCE_DESC ResourceDescriptor::native() const
{
    return D3D12_RESOURCE_DESC{
        static_cast<D3D12_RESOURCE_DIMENSION>(dimension),
        static_cast<UINT64>(alignment),
        width, height, depth, num_mipmaps, format,
        DXGI_SAMPLE_DESC{multisampling_format.count, multisampling_format.quality},
        static_cast<D3D12_TEXTURE_LAYOUT>(layout),
        static_cast<D3D12_RESOURCE_FLAGS>(flags.getValue())
    };
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


ResourceDescriptor ResourceDescriptor::CreateTexture1D(uint64_t width, uint16_t array_size, 
    DXGI_FORMAT format, uint16_t num_mipmaps, ResourceFlags flags,
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

ResourceDescriptor ResourceDescriptor::CreateTexture2D(uint64_t width, uint32_t height, 
    uint16_t array_size, DXGI_FORMAT format, uint16_t num_mipmaps,
    ResourceFlags flags, MultiSamplingFormat ms_format, 
    ResourceAlignment alignment, TextureLayout layout)
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

ResourceDescriptor ResourceDescriptor::CreateTexture3D(uint64_t width, uint32_t height, 
    uint16_t depth, DXGI_FORMAT format, uint16_t num_mipmaps, ResourceFlags flags, 
    MultiSamplingFormat ms_format, ResourceAlignment alignment, TextureLayout layout)
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

Resource::Resource(Heap& heap, uint64_t heap_offset, 
    ResourceState const& initial_state, 
    misc::Optional<ResourceOptimizedClearValue> const& optimized_clear_value, 
    ResourceDescriptor const& descriptor):
    m_heap{ heap },
    m_offset{ heap_offset },
    m_descriptor{ descriptor }
{
    D3D12_RESOURCE_DESC desc = descriptor.native();

    D3D12_CLEAR_VALUE native_clear_value{};
    D3D12_CLEAR_VALUE* native_clear_value_ptr = nullptr;
    if (optimized_clear_value.isValid())
    {
        native_clear_value = static_cast<ResourceOptimizedClearValue const&>(optimized_clear_value).native();
        native_clear_value_ptr = &native_clear_value;
    }

    LEXGINE_THROW_ERROR_IF_FAILED(this,
        heap.device().native()->CreatePlacedResource(heap.native().Get(), heap_offset, &desc, static_cast<D3D12_RESOURCE_STATES>(initial_state.getValue()), native_clear_value_ptr, IID_PPV_ARGS(&m_resource)),
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

ResourceDescriptor const& Resource::descriptor() const
{
    return m_descriptor;
}

void* Resource::map(unsigned int subresource/* = 0U */, 
    size_t offset/* = 0U */, size_t mapping_range/* = static_cast<size_t>(-1) */) const
{
    D3D12_RESOURCE_DESC desc = m_descriptor.native();
    uint64_t total_resource_size_in_bytes{ 0 };
    m_heap.device().native()->GetCopyableFootprints(&desc, subresource, 1, 0UI64, nullptr, nullptr, nullptr, &total_resource_size_in_bytes);

    D3D12_RANGE read_range{ offset, (std::min<uint64_t>)(total_resource_size_in_bytes, mapping_range) };
    void* rv{ nullptr };
    LEXGINE_THROW_ERROR_IF_FAILED(this,
        m_resource->Map(static_cast<UINT>(subresource), &read_range, &rv),
        S_OK
    );

    return rv;
}

void Resource::unmap(unsigned int subresource/* = 0U */) const
{
    m_resource->Unmap(static_cast<UINT>(subresource), nullptr);
}

DepthStencilValue::DepthStencilValue(float depth, uint8_t stencil):
    depth{ depth },
    stencil{ stencil }
{
}

D3D12_DEPTH_STENCIL_VALUE DepthStencilValue::native() const
{
    D3D12_DEPTH_STENCIL_VALUE rv{};

    rv.Depth = static_cast<FLOAT>(depth);
    rv.Stencil = static_cast<UINT8>(stencil);

    return rv;
}

ResourceOptimizedClearValue::ResourceOptimizedClearValue(DXGI_FORMAT format, math::Vector4f const& color):
    format{ format },
    value{ color }
{
}

ResourceOptimizedClearValue::ResourceOptimizedClearValue(DXGI_FORMAT format, DepthStencilValue const& depth_stencil_value):
    format{ format },
    value{ depth_stencil_value }
{
}

D3D12_CLEAR_VALUE ResourceOptimizedClearValue::native() const
{
    D3D12_CLEAR_VALUE rv{};
    rv.Format = format;
    
    if (std::holds_alternative<math::Vector4f>(value))
        memcpy(rv.Color, std::get<math::Vector4f>(value).getDataAsArray(), sizeof(float) * 4);
    else
        rv.DepthStencil = std::get<DepthStencilValue>(value).native();

    return rv;
}
