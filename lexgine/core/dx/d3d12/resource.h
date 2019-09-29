#ifndef LEXGINE_CORE_DX_D3D12_RESOURCE_H
#define LEXGINE_CORE_DX_D3D12_RESOURCE_H

#include <variant>

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/multisampling.h"
#include "lexgine/core/misc/flags.h"
#include "lexgine/core/misc/optional.h"
#include "lexgine/core/math/vector_types.h"
#include "heap.h"


namespace lexgine::core::dx::d3d12 {

//! Possible dimensions of resources
enum class ResourceDimension
{
    unknown,
    buffer,
    texture1d,
    texture2d,
    texture3d
};


//! Allowed resource alignment (the choice depends on the target usage of resource being allocated)
enum class ResourceAlignment : uint64_t
{
    default = 0,    //! use 4MB alignment for MSAA resources and 64KB alignment for everything else
    _4kb = 4096U,    //!< 4KB boundary (may be used by smaller textures)
    _64kb = 65536U,    //!< 64KB boundary (default for most resources)
    _4MB = 4194304U    //!< 4MB boundary (used for MSAA resources)
};


//! These are low-level texture layout representation hints
//! Exact choice is to be made depending on the kind of resource being created
enum class TextureLayout
{
    unknown,    //!< driver specific
    row_major,   //!< textures are stored in row-major order with padding. Enable cross-adapter sharing of texture data
    _64kb_undefined_swizzle,    //!< tiled resource: the swizzling within tiles is driver-specific
    _64kb_standard_swizzle    //!< tiled resource: the swizzling is well-defined, thence optimizations are possible when marshaling the data between multiple GPUs or between CPU and GPU
};


//! Resource flags
BEGIN_FLAGS_DECLARATION(ResourceFlags)
FLAG(none, 0)
FLAG(render_target, 0x1)
FLAG(depth_stencil, 0x2)
FLAG(unordered_access, 0x4)
FLAG(deny_shader_resource, 0x8)
FLAG(allow_cross_adapter, 0x10)
FLAG(allow_simultaneous_access, 0x20)
END_FLAGS_DECLARATION(ResourceFlags)


//! Thin class wrapper over D3D12_RESOURCE_DESC to simplify its creation
struct ResourceDescriptor
{
    ResourceDimension dimension;    //!< dimension of the resource
    ResourceAlignment alignment;    //!< memory alignment of the resource
    uint64_t width;    //!< resource width
    uint32_t height;    //!< resource height (ignored for 1D textures and buffers)
    uint16_t depth;    //!< resource depth (the depth for 3D textures or array size for array textures, ignored for other resource types)
    uint16_t num_mipmaps;    //!< number of mipmap levels in the resource (ignored for buffers)
    DXGI_FORMAT format;    //!< DXGI format, in which the resource is represented
    MultiSamplingFormat multisampling_format;    //!< format of the multi-sampling.
    TextureLayout layout;    //!< layout of the texture. Exact choices are to be made by particular type of the resource being created
    ResourceFlags flags;    //!< creation flags

    //! Returns size allocated by the resource in GPU memory. The size is driver dependent and thus in order to be determined properly
    //! it requires knowledge about device interface and about the nodes to which the resource is exposed, as both these parameters may in theory affect the final size
    uint64_t getAllocationSize(Device const& device, uint32_t node_exposure_mask) const;

    D3D12_RESOURCE_DESC native() const;    // returns native D3D12 resource descriptor struct

    static ResourceDescriptor CreateBuffer(uint64_t size, ResourceFlags flags = ResourceFlags::base_values::none);    //! fills out the fields of the structure as required for buffers

    static ResourceDescriptor CreateTexture1D(uint64_t width, uint16_t array_size, DXGI_FORMAT format, uint16_t num_mipmaps = 1, ResourceFlags flags = ResourceFlags::base_values::none,
        MultiSamplingFormat ms_format = MultiSamplingFormat{ 1, 0 }, ResourceAlignment alignment = ResourceAlignment::default,
        TextureLayout layout = TextureLayout::unknown);    //! fills out the fields of the structure as required for 1D textures

    static ResourceDescriptor CreateTexture2D(uint64_t width, uint32_t height, uint16_t array_size, DXGI_FORMAT format, uint16_t num_mipmaps = 1, ResourceFlags flags = ResourceFlags::base_values::none,
        MultiSamplingFormat ms_format = MultiSamplingFormat{ 1, 0 }, ResourceAlignment alignment = ResourceAlignment::default,
        TextureLayout layout = TextureLayout::unknown);    //! fills out the fields of the structure as required for 2D textures

    static ResourceDescriptor CreateTexture3D(uint64_t width, uint32_t height, uint16_t depth, DXGI_FORMAT format, uint16_t num_mipmaps = 1, ResourceFlags flags = ResourceFlags::base_values::none,
        MultiSamplingFormat ms_format = MultiSamplingFormat{ 1, 0 }, ResourceAlignment alignment = ResourceAlignment::default,
        TextureLayout layout = TextureLayout::unknown);    //! fills out the fields of the structure as required for 3D textures
};


//! Resource states
BEGIN_FLAGS_DECLARATION(ResourceState)
FLAG(common, D3D12_RESOURCE_STATE_COMMON)
FLAG(vertex_and_constant_buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
FLAG(index_buffer, D3D12_RESOURCE_STATE_INDEX_BUFFER)
FLAG(render_target, D3D12_RESOURCE_STATE_RENDER_TARGET)
FLAG(unordered_access, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
FLAG(depth_write, D3D12_RESOURCE_STATE_DEPTH_WRITE)
FLAG(depth_read, D3D12_RESOURCE_STATE_DEPTH_READ)
FLAG(non_pixel_shader, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
FLAG(pixel_shader, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
FLAG(stream_out, D3D12_RESOURCE_STATE_STREAM_OUT)
FLAG(indirect_argument, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
FLAG(copy_destination, D3D12_RESOURCE_STATE_COPY_DEST)
FLAG(copy_source, D3D12_RESOURCE_STATE_COPY_SOURCE)
FLAG(resolve_destination, D3D12_RESOURCE_STATE_RESOLVE_DEST)
FLAG(resolve_source, D3D12_RESOURCE_STATE_RESOLVE_SOURCE)
FLAG(generic_read, D3D12_RESOURCE_STATE_GENERIC_READ)
FLAG(present, D3D12_RESOURCE_STATE_PRESENT)
FLAG(predication, D3D12_RESOURCE_STATE_PREDICATION)
END_FLAGS_DECLARATION(ResourceState)


struct DepthStencilValue final
{
    float depth;
    uint8_t stencil;

    DepthStencilValue(float depth, uint8_t stencil);

    D3D12_DEPTH_STENCIL_VALUE native() const;
};

struct ResourceOptimizedClearValue final
{
    DXGI_FORMAT format;
    std::variant<math::Vector4f, DepthStencilValue> value;

    ResourceOptimizedClearValue(DXGI_FORMAT format, math::Vector4f const& color);
    ResourceOptimizedClearValue(DXGI_FORMAT format, DepthStencilValue const& depth_stencil_value);

    D3D12_CLEAR_VALUE native() const;
};


class Resource : public NamedEntity<class_names::D3D12_Resource>
{
public:
    Resource(ComPtr<ID3D12Resource> const& native = nullptr);
    Resource(ResourceState const& initial_state, ComPtr<ID3D12Resource> const& native = nullptr);
    virtual ~Resource() = default;

    void setStringName(std::string const& entity_string_name) override;

    ComPtr<ID3D12Resource> native() const;    //! returns encapsulated native Direct3D12 interface representing the resource

    //! maps resource to the CPU-memory. Call to this function will fail if the resource's heap does not support CPU access
    void* map(unsigned int subresource = 0U,
        size_t offset = 0U, size_t mapping_range = static_cast<size_t>(-1)) const;

    void unmap(unsigned int subresource = 0U, bool mapped_data_was_modified = true) const;    //! unmaps resource from the CPU memory

    uint64_t getGPUVirtualAddress() const;

    ResourceDescriptor const& descriptor() const;    //! returns descriptor of the resource

    ResourceState const& defaultState() const { return m_resource_default_state; }

protected:
    ComPtr<ID3D12Resource> m_resource;    //!< encapsulated native interface representing the resource
    ResourceState m_resource_default_state;    //!< default state of the resource, with which it was created
    misc::Optional<ResourceDescriptor> mutable m_descriptor;    //!< resource descriptor
};


//! Wrapper over placed resource
class PlacedResource final : public Resource
{
public:
    /*! Creates placed resource in provided @param heap at the given @param offset. Note that @param initial_state and @param optimized_clear_value
     may be overridden to certain values depending on the type of the heap and on the dimension of the resource being created. THROWS
     */
    PlacedResource(Heap const& heap, uint64_t heap_offset, ResourceState const& initial_state,
        misc::Optional<ResourceOptimizedClearValue> const& optimized_clear_value, ResourceDescriptor const& descriptor);

    Heap const& heap() const;    //! returns the heap, in which the resource resides
    uint64_t offset() const;    //! returns offset to the resource in the owning heap    

private:
    Heap const& m_heap;    //!< reference to the heap, in which the resource resides
    uint64_t m_offset;    //!< offset to the resource in the owning heap
};


//! Wrapper over committed resource
class CommittedResource final : public Resource
{
public:
    CommittedResource(Device const& device, ResourceState initial_state,
        misc::Optional<ResourceOptimizedClearValue> const& optimized_clear_value,
        ResourceDescriptor const& descriptor, AbstractHeapType resource_memory_type,
        HeapCreationFlags resource_usage_flags,
        uint32_t node_mask = 0x1, uint32_t node_exposure_mask = 0x1);

    CommittedResource(Device const& device, ResourceState initial_state,
        misc::Optional<ResourceOptimizedClearValue> const& optimized_clear_value,
        ResourceDescriptor const& descriptor, CPUPageProperty cpu_page_property,
        GPUMemoryPool gpu_memory_pool, HeapCreationFlags resource_usage_flags,
        uint32_t node_mask = 0x1, uint32_t node_exposure_mask = 0x1);

private:
    void createResource(D3D12_HEAP_PROPERTIES const& owning_heap_properties,
        ResourceState resource_init_state, ResourceDescriptor const& resource_desc,
        HeapCreationFlags resource_usage_flags, misc::Optional<ResourceOptimizedClearValue> const& optimized_clear_value);

private:
    Device const& m_device;
    CPUPageProperty m_cpu_page_property;
    GPUMemoryPool m_gpu_memory_pool;
    uint32_t m_node_mask;
    uint32_t m_node_exposure_mask;
};




}


#endif
