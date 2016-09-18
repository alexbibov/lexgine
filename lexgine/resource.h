#ifndef LEXGINE_CORE_DX_D3D12_RESOURCE_H

#include "entity.h"
#include "class_names.h"
#include "heap.h"
#include "multisampling.h"
#include "flags.h"



namespace lexgine {namespace core {namespace dx {namespace d3d12 {

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
    _4kb = 4096U,    //!< 4KB boundary (may be used by smaller textures and get eventually mimicked by tiled representation)
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


namespace __tag {
enum class tagResourceFlags
{
    none = 0,
    render_target = 0x1,
    depth_stencil = 0x2,
    unordered_access = 0x4,
    deny_shader_resource = 0x8,
    allow_cross_adapter = 0x10,    // note that this precludes usage of mip-maps
    allow_simultaneous_access = 0x20
};
}


using ResourceFlags = misc::Flags<__tag::tagResourceFlags>;    //! resource creation flags


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

    static ResourceDescriptor CreateBuffer(uint64_t size, ResourceFlags flags = ResourceFlags{ ResourceFlags::enum_type::none });    //! fills out the fields of the structure as required for buffers

    static ResourceDescriptor CreateTexture1D(uint64_t width, uint16_t array_size, DXGI_FORMAT format, uint16_t num_mipmaps = 1, ResourceFlags flags = ResourceFlags{ ResourceFlags::enum_type::none },
        MultiSamplingFormat ms_format = MultiSamplingFormat{ 1, 0 }, ResourceAlignment alignment = ResourceAlignment::default,
        TextureLayout layout = TextureLayout::unknown);    //! fills out the fields of the structure as required for 1D textures

    static ResourceDescriptor CreateTexture2D(uint64_t width, uint32_t height, uint16_t array_size, DXGI_FORMAT format, uint16_t num_mipmaps = 1, ResourceFlags flags = ResourceFlags{ ResourceFlags::enum_type::none },
        MultiSamplingFormat ms_format = MultiSamplingFormat{ 1, 0 }, ResourceAlignment alignment = ResourceAlignment::default,
        TextureLayout layout = TextureLayout::unknown);    //! fills out the fields of the structure as required for 2D textures

    static ResourceDescriptor CreateTexture3D(uint64_t width, uint32_t height, uint16_t depth, DXGI_FORMAT format, uint16_t num_mipmaps = 1, ResourceFlags flags = ResourceFlags{ ResourceFlags::enum_type::none },
        MultiSamplingFormat ms_format = MultiSamplingFormat{ 1, 0 }, ResourceAlignment alignment = ResourceAlignment::default,
        TextureLayout layout = TextureLayout::unknown);    //! fills out the fields of the structure as required for 3D textures
};


//! Resource states
namespace __tag
{
enum class tagResourceState
{
    common = D3D12_RESOURCE_STATE_COMMON,
    vertex_and_constant_buffer = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
    index_buffer = D3D12_RESOURCE_STATE_INDEX_BUFFER,
    render_target = D3D12_RESOURCE_STATE_RENDER_TARGET,
    unordered_access = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    depth_write = D3D12_RESOURCE_STATE_DEPTH_WRITE,
    depth_read = D3D12_RESOURCE_STATE_DEPTH_READ,
    non_pixel_shader = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    pixel_shader = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    stream_out = D3D12_RESOURCE_STATE_STREAM_OUT,
    indirect_argument = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
    copy_destination = D3D12_RESOURCE_STATE_COPY_DEST,
    copy_source = D3D12_RESOURCE_STATE_COPY_SOURCE,
    resolve_destination = D3D12_RESOURCE_STATE_RESOLVE_DEST,
    resolve_source = D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
    generic_read = D3D12_RESOURCE_STATE_GENERIC_READ,
    present = D3D12_RESOURCE_STATE_PRESENT,
    predication = D3D12_RESOURCE_STATE_PREDICATION
};
}


using ResourceState = misc::Flags<__tag::tagResourceState>;


//! Wrapper over placed resource context
class Resource final : public NamedEntity<class_names::D3D12Resource>
{
    template<size_t> friend class ResourceBarrier;    // resource state transitions are allowed to change the current resource state, which is otherwise hidden

public:
    //! Creates placed resource in provided @param heap at the given @param offset. Note that @param initial_state and @param optimized_clear_value
    //! may be overridden to certain values depending on the type of the heap and on the dimension of the resource being created
    Resource(Heap& heap, uint64_t heap_offset, ResourceState const& initial_state, D3D12_CLEAR_VALUE const& optimized_clear_value, ResourceDescriptor const& descriptor);


    // Resources are the only copyable objects at ring0. The copy is always shallow as the object is merely wrapper over the
    // memory occupied by the resource

    Resource(Resource const&) = default;
    Resource(Resource&&) = default;

    Heap& heap() const;    //! returns the heap, in which the resource resides
    uint64_t offset() const;    //! returns offset to the resource in the owning heap

    ComPtr<ID3D12Resource> native() const;    //! returns encapsulated native Direct3D12 interface representing the resource

private:
    Heap& m_heap;    //!< reference to the heap, in which the resource resides
    uint64_t m_offset;    //!< offset to the resource in the owning heap
    ResourceState m_state;    //!< current state of the resource

    ComPtr<ID3D12Resource> m_resource;    //!< encapsulated native interface representing the resource
};

}}}}

#define LEXGINE_CORE_DX_D3D12_RESOURCE_H
#endif
