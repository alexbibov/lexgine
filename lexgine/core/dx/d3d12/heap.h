#ifndef LEXGINE_CORE_DX_D3D12_HEAP_H
#define LEXGINE_CORE_DX_D3D12_HEAP_H

#include <d3d12.h>
#include <wrl.h>

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/misc/flags.h"


using namespace Microsoft::WRL;

namespace lexgine::core::dx::d3d12 {

namespace __tag {
enum class tagHeapCreationFlags
{
    shared = 0x1,    //!< the heap can be shared between multiple processes
    deny_buffers = 0x4,    //!< the heap does not allow to store buffer resources
    shared_cross_adapter = 0x20,    //!< the heap is shared between adapters (avoids CPU marshaling of the heap)
    deny_rt_ds = 0x40,    //!< denies all texture render target and depth-stencil resources
    deny_non_rt_ds = 0x80,    //!< denies all textures that are not either render target or depth-stencil resources. Buffers are allowed.
    allow_all = 0,    //!< allows all kind of resources to be stored in the heap. Requires Heap tier 2.
    allow_only_buffers = 0xc0,    //!< allows only buffers
    allow_only_non_rt_ds_textures = 0x44,    //!< allows only textures (not buffers!) that are not render targets and not depth-stencil resources
    allow_only_rt_ds = 0x84    //!< allows to store only textures (not buffers!) that are either render target or depth-stencil resources
};
}

using HeapCreationFlags = misc::Flags<__tag::tagHeapCreationFlags>;    //! flags determining which resources may be located within the heap

//! Determines abstract adapter-agnostic type of the heap
enum class AbstractHeapType
{
    default = 1,    //<! default heap. The heap allocated from this pool will experience the most bandwidth for the GPU, but would not allow CPU access
    upload = 2,    //<! upload heap optimized for uploading data to the GPU-side. The best usage scenario is CPU-write-once & GPU-read-once cases
    readback = 3    //!< optimized for reading data back from the GPU-side
};

//! CPU page property type
enum class CPUPageProperty
{
    not_available = 1,    //!< CPU writes are not allowed and thus the memory page properties are not available
    write_combine = 2,    //!< CPU-page property is write-combined
    write_back = 3    //!< CPU-page property is write-back
};

//! GPU memory pool type
enum class GPUMemoryPool
{
    L0 = 1,    //!< system memory pool. For UMA adapters this pool is the only available
    L1 = 2    //!< GPU physical memory pool, available only in NUMA adapters
};

//! Wrapper over Direct3D heap
class Heap : public NamedEntity<class_names::D3D12_Heap>
{
    friend class Device;    // only devices are allowed to create heaps

public:

    Device& device() const;    //! returns device used to create this heap
    size_t capacity() const;    //! returns capacity of the heap in bytes
    ComPtr<ID3D12Heap> native() const;    //! returns native heap
    bool isMSAASupported() const;    //! returns 'true' if the heap is capable of storing MSAA resources
    CPUPageProperty getCPUPage() const;    //! returns CPU page property determining CPU access rights for the heap
    GPUMemoryPool getGPUMemoryPool() const;    //! returns GPU memory pool where this heap is located

    uint32_t getResidentDevice() const;    //! returns bitmask with exactly one bit set correspondingly to the physical adapter that owns the heap
    uint32_t getExposureMask() const;    //! returns bitmask with those bits set that correspond to the physical adapters that have access to the heap


    Heap(Heap const&) = delete;
    Heap(Heap&&) = default;

private:
    //! Initializes heap with provided abstract type, which allows to disregard hardware specifics. THROWS
    Heap(Device& device, AbstractHeapType type, uint64_t size, HeapCreationFlags flags, bool is_msaa_supported, uint32_t node_mask, uint32_t node_exposure_mask);

    /*! Initialized custom heap with the given CPU page and GPU memory pool parameters. Note that not all combinations of
     parameters are valid on all systems. Request adapter architecture details using Device object. THROWS
    */
    Heap(Device& device, CPUPageProperty cpu_page_property, GPUMemoryPool gpu_memory_pool, uint64_t size, HeapCreationFlags flags, bool is_msaa_supported, uint32_t node_mask, uint32_t node_exposure_mask);


    Device& m_device;    //!< device used to create this heap
    uint64_t m_size;    //!< size of the heap in bytes
    bool m_is_msaa_supported;    //!< 'true' when the heap can store MSAA resources
    ComPtr<ID3D12Heap> m_heap;    //!< native Direct3D 12 heap interface
    CPUPageProperty m_cpu_page;    //!< CPU page property determining CPU access rights for the heap
    GPUMemoryPool m_gpu_pool;    //!< GPU memory pool where the heap is located
    uint32_t m_node_mask;    //!< node mask determining at which node the resource physically resides
    uint32_t m_node_exposure_mask;    //!< mask with those bits set that correspond to the physical adapters, for which the heap will be visible
};

}

#endif
