#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_HEAP_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_HEAP_H

#include "engine/core/entity.h"
#include "engine/core/class_names.h"

#include "lexgine_core_dx_d3d12_fwd.h"

#include <d3d12.h>
#include <wrl.h>
#include <atomic>

using namespace Microsoft::WRL;

namespace lexgine::core::dx::d3d12 {

enum class DescriptorHeapType {
    cbv_srv_uav,
    sampler,
    rtv,
    dsv,
    count
};


struct DescriptorTable final {
    size_t offset;                   // number of descriptor-sized slots preceding this descriptor table
    uintptr_t cpu_pointer;           // CPU pointer to the start of descriptor table
    uint64_t gpu_pointer;            // GPU pointer to the start of descriptor table
    uint32_t descriptor_count;       // descriptor capacity of the table
    uint32_t descriptor_size;        // size of singe descriptor
    DescriptorHeap* p_heap;          // pointer to owning heap
};

class DescriptorHeap final : public NamedEntity<class_names::D3D12_DescriptorHeap>
{
    friend class Device;    // only devices are allowed to create the heaps

public:
    Device& device() const;    //! returns the device used to create this descriptor heap
    ComPtr<ID3D12DescriptorHeap> native() const;    //! returns encapsulated reference to the native Direct3D 12 interface representing the descriptor heap
    
    uint32_t getDescriptorSize() const;    //! returns size occupied in GPU memory by single descriptor
    size_t getBaseCPUPointer() const;    //! returns base CPU pointer of the descriptor heap
    uint64_t getBaseGPUPointer() const;    //! returns base GPU pointer of the descriptor heap
    bool isGpuVisible() const { return m_is_gpu_visible; } //! returns true if the descriptor heap is visible from GPU side

    DescriptorHeap(DescriptorHeap const&) = delete;
    DescriptorHeap(DescriptorHeap&&) = default;

    DescriptorHeapType type() const { return m_type; }

    void reset();    // resets the descriptor heap

    uint32_t capacity() const;

    
    uint32_t descriptorsAllocated() const;    //! descriptor count reserved in the descriptor heap by the moment this function was invoked

    uint32_t reserveDescriptors(uint32_t count);    //! reserves "count" descriptors in the descriptor heap and returns offset of the first descriptor reserved

    DescriptorTable allocateDescriptorTable(uint32_t capacity);

    /*! creates single CBV descriptor and places it into descriptor heap starting at position provided by
     the offset value. The offset can be obtained using reserveDescriptors(...).
     The return value of this function is GPU address of the created descriptor.
    */
    uint64_t createConstantBufferViewDescriptor(size_t offset, CBVDescriptor const& cbv_descriptor);

    /*! creates CBV descriptors and places them into this descriptor heap beginning from position determined by
     provided offset value. The offset can be obtained using reserveDescriptors(...).
     The return value of this function is GPU address of the first created descriptor.
    */
    uint64_t createConstantBufferViewDescriptors(size_t offset, std::vector<CBVDescriptor> const& cbv_descriptors);



    /*! creates single SRV descriptor and places it into this descriptor heap beginning from position determined by
    provided offset value. The offset can be obtained using reserveDescriptors(...).
    The return value of this function is GPU address of the created descriptor.
   */
    uint64_t createShaderResourceViewDescriptor(size_t offset, SRVDescriptor const& srv_descriptor);

    /*! creates SRV descriptors and places them into this descriptor heap beginning from position determined by
     provided offset value. The offset can be obtained using reserveDescriptors(...).
     The return value of this function is GPU address of the first created descriptor.
    */
    uint64_t createShaderResourceViewDescriptors(size_t offset, std::vector<SRVDescriptor> const& srv_descriptors);



    /*! creates single UAV descriptor and places it into this descriptor heap beginning from position determined by
     provided offset value. The offset can be obtained using reserveDescriptors(...).
     The return value of this function is GPU address of the created descriptor.
    */
    uint64_t createUnorderedAccessViewDescriptor(size_t offset, UAVDescriptor const& uav_descriptor);

    /*! creates UAV descriptors and places them into this descriptor heap beginning from position determined by
     provided offset value. The offset can be obtained using reserveDescriptors(...).
     The return value of this function is GPU address of the first created descriptor.
    */
    uint64_t createUnorderedAccessViewDescriptors(size_t offset, std::vector<UAVDescriptor> const& uav_descriptors);



    /*! creates single sampler descriptor and places it into descriptor heap starting at position determined by
     provided offset value. The offset can be obtained using reserveDescriptors(...).
     The return value of this function is GPU address of the created descriptor.
    */
    uint64_t createSamplerDescriptor(size_t offset, SamplerDescriptor const& sampler_descriptor);

    /*! creates sampler descriptors and places them into this descriptor heap beginning from position determined by
     provided offset value. The offset can be obtained using reserveDescriptors(...).
     The return value of this function is GPU address of the first created descriptor.
    */
    uint64_t createSamplerDescriptors(size_t offset, std::vector<SamplerDescriptor> const& sampler_descriptors);

  

    /*! creates single RTV descriptor and places it into descriptor heap starting at position provided by
     the offset value. The offset can be obtained using reserveDescriptors(...).
     The return value of this function is GPU address of the created descriptor
    */
    uint64_t createRenderTargetViewDescriptor(size_t offset, RTVDescriptor const& rtv_descriptor);
    
    /*! creates RTV descriptors and places them into this descriptor heap beginning from position determined by
     provided offset value. The offset can be obtained using reserveDescriptors(...).
     The return value of this function is GPU address of the first created descriptor.
    */
    uint64_t createRenderTargetViewDescriptors(size_t offset, std::vector<RTVDescriptor> const& rtv_descriptors);



    /*! creates single DSV descriptor and places it into descriptor heap starting at position provided by
     offset value. The offset can be obtained using reserveDescriptors(...).
     The return value of this function is GPU address of the created descriptor.
    */
    uint64_t createDepthStencilViewDescriptor(size_t offset, DSVDescriptor const& dsv_descriptor);

    /*! creates DSV descriptors and places them into this descriptor heap beginning from position determined by
     provided offset value. The offset can be obtained using reserveDescriptors(...).
     The return value of this function is GPU address of the first created descriptor.
    */
    uint64_t createDepthStencilViewDescriptors(size_t offset, std::vector<DSVDescriptor> const& dsv_descriptors);

private:
    DescriptorHeap(Device& device, DescriptorHeapType type, uint32_t descriptor_capacity, uint32_t node_mask);

    Device& m_device;    //!< device that has created this heap
    ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;    //!< reference to the native Direct3D 12 descriptor heap interface
    DescriptorHeapType m_type;    //!< type of the descriptor heap
    uint32_t const m_descriptor_size;    //!< size of a single descriptor in the heap
    uint32_t const m_descriptor_capacity;    //!< number of descriptors that could be stored in the heap
    uint32_t m_node_mask;    //!< mask determining adapter, to which the heap is assigned
    std::atomic<uint32_t> m_num_descriptors_allocated;    //!< number of currently allocated descriptors

    size_t m_heap_start_cpu_address;    //!< CPU address of the beginning of the heap
    uint64_t m_heap_start_gpu_address;    //!< GPU address of the beginning of the heap
    bool m_is_gpu_visible{ false };    //!< true if descriptor heap is visible from GPU side
};



}

#endif
