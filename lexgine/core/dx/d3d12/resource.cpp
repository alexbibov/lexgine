#include <algorithm>
#include <cassert>

#include "resource.h"
#include "device.h"
#include "lexgine/core/exception.h"

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



Resource::Resource(ComPtr<ID3D12Resource> const& native)
    : m_resource{ native }
{
}

Resource::Resource(ResourceState const& initial_state, ComPtr<ID3D12Resource> const& native/* = nullptr */)
    : m_resource{ native }
    , m_resource_default_state{ initial_state }
{
}

void Resource::setStringName(std::string const& entity_string_name)
{
    Entity::setStringName(entity_string_name);
    m_resource->SetName(misc::asciiStringToWstring(entity_string_name).c_str());
}

ComPtr<ID3D12Resource> Resource::native() const
{
    return m_resource;
}

void* Resource::map(unsigned int subresource/* = 0U */,
    size_t offset/* = 0U */, size_t mapping_range/* = static_cast<size_t>(-1) */) const
{
    assert(m_resource != nullptr);

    ComPtr<ID3D12Device> native_device;
    m_resource->GetDevice(IID_PPV_ARGS(&native_device));

    D3D12_RESOURCE_DESC desc = descriptor().native();
    uint64_t total_resource_size_in_bytes{ 0 };
    native_device->GetCopyableFootprints(&desc, subresource, 1, 0UI64, nullptr, nullptr, nullptr, &total_resource_size_in_bytes);

    D3D12_RANGE read_range{ offset, (std::min<uint64_t>)(total_resource_size_in_bytes, mapping_range) };
    void* rv{ nullptr };
    LEXGINE_THROW_ERROR_IF_FAILED(this,
        m_resource->Map(static_cast<UINT>(subresource), &read_range, &rv),
        S_OK
    );

    return rv;
}

void Resource::unmap(unsigned int subresource/* = 0U */, bool mapped_data_was_modified/* = true*/) const
{
    assert(m_resource != nullptr);
    D3D12_RANGE no_modification_range{};
    m_resource->Unmap(static_cast<UINT>(subresource), mapped_data_was_modified ? nullptr : &no_modification_range);
}

uint64_t Resource::getGPUVirtualAddress() const
{
    assert(m_resource != nullptr);

    return m_resource->GetGPUVirtualAddress();
}

ResourceDescriptor const& Resource::descriptor() const
{
    assert(m_resource != nullptr);

    if (!m_descriptor.isValid())
    {
        auto native_descriptor = m_resource->GetDesc();
        ResourceDescriptor desc;
        desc.dimension = static_cast<ResourceDimension>(native_descriptor.Dimension);
        desc.alignment = static_cast<ResourceAlignment>(native_descriptor.Alignment);
        desc.width = native_descriptor.Width;
        desc.height = native_descriptor.Height;
        desc.depth = native_descriptor.DepthOrArraySize;
        desc.num_mipmaps = native_descriptor.MipLevels;
        desc.format = native_descriptor.Format;
        desc.multisampling_format.quality = native_descriptor.SampleDesc.Quality;
        desc.multisampling_format.count = native_descriptor.SampleDesc.Count;
        desc.layout = static_cast<TextureLayout>(native_descriptor.Layout);
        desc.flags = native_descriptor.Flags;

        m_descriptor = desc;
    }

    return static_cast<ResourceDescriptor const&>(m_descriptor);
}




PlacedResource::PlacedResource(Heap const& heap, uint64_t heap_offset,
    ResourceState const& initial_state,
    misc::Optional<ResourceOptimizedClearValue> const& optimized_clear_value,
    ResourceDescriptor const& descriptor) 
    : Resource{ initial_state }
    , m_heap{ heap }
    , m_offset{ heap_offset }
{
    m_descriptor = descriptor;
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

Heap const& PlacedResource::heap() const
{
    return m_heap;
}

uint64_t PlacedResource::offset() const
{
    return m_offset;
}





DepthStencilValue::DepthStencilValue(float depth, uint8_t stencil) :
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

ResourceOptimizedClearValue::ResourceOptimizedClearValue(DXGI_FORMAT format, math::Vector4f const& color) :
    format{ format },
    value{ color }
{
}

ResourceOptimizedClearValue::ResourceOptimizedClearValue(DXGI_FORMAT format, DepthStencilValue const& depth_stencil_value) :
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

CommittedResource::CommittedResource(Device const& device, ResourceState initial_state,
    misc::Optional<ResourceOptimizedClearValue> const& optimized_clear_value,
    ResourceDescriptor const& descriptor, AbstractHeapType resource_memory_type,
    HeapCreationFlags resource_usage_flags,
    uint32_t node_mask/* = 0x1*/, uint32_t node_exposure_mask/* = 0x1*/)
    : Resource{ initial_state }
    , m_device{ device }
    , m_node_mask{ node_mask }
    , m_node_exposure_mask{ node_exposure_mask }
{
    Heap::retrieveAbstractHeapTypeProperties(device, resource_memory_type, node_mask, m_cpu_page_property, m_gpu_memory_pool);
    D3D12_HEAP_PROPERTIES heap_properties = Heap::createNativeHeapProperties(resource_memory_type, node_mask, node_exposure_mask);
    createResource(heap_properties, initial_state, descriptor, resource_usage_flags, optimized_clear_value);
}

CommittedResource::CommittedResource(Device const& device, ResourceState initial_state,
    misc::Optional<ResourceOptimizedClearValue> const& optimized_clear_value,
    ResourceDescriptor const& descriptor, CPUPageProperty cpu_page_property,
    GPUMemoryPool gpu_memory_pool, HeapCreationFlags resource_usage_flags,
    uint32_t node_mask/* = 0x1*/, uint32_t node_exposure_mask/* = 0x1*/)
    : Resource{ initial_state }
    , m_device{ device }
    , m_cpu_page_property{ cpu_page_property }
    , m_gpu_memory_pool{ gpu_memory_pool }
    , m_node_mask{ node_mask }
    , m_node_exposure_mask{ node_exposure_mask }
{
    D3D12_HEAP_PROPERTIES heap_properties = Heap::createNativeHeapProperties(cpu_page_property, gpu_memory_pool, node_mask, node_exposure_mask);
    createResource(heap_properties, initial_state, descriptor, resource_usage_flags, optimized_clear_value);
}

void CommittedResource::createResource(D3D12_HEAP_PROPERTIES const& owning_heap_properties,
    ResourceState resource_init_state, ResourceDescriptor const& resource_desc,
    HeapCreationFlags resource_usage_flags, misc::Optional<ResourceOptimizedClearValue> const& optimized_clear_value)
{
    D3D12_HEAP_FLAGS flags = static_cast<D3D12_HEAP_FLAGS>(resource_usage_flags.getValue());
    D3D12_RESOURCE_DESC native_resource_desc = resource_desc.native();

    D3D12_CLEAR_VALUE resource_clear_value{};
    D3D12_CLEAR_VALUE* resource_clear_value_ptr{ nullptr };
    if (optimized_clear_value.isValid())
    {
        resource_clear_value = static_cast<ResourceOptimizedClearValue const&>(optimized_clear_value).native();
        resource_clear_value_ptr = &resource_clear_value;
    }

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_device.native()->CreateCommittedResource(&owning_heap_properties, flags, &native_resource_desc,
            static_cast<D3D12_RESOURCE_STATES>(resource_init_state.getValue()), resource_clear_value_ptr,
            IID_PPV_ARGS(&m_resource)),
        S_OK);
}
