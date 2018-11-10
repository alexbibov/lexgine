#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_HEAP_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_HEAP_H

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"

#include "lexgine_core_dx_d3d12_fwd.h"

#include <d3d12.h>
#include <wrl.h>
#include <atomic>

using namespace Microsoft::WRL;

namespace lexgine::core::dx::d3d12 {

//! Descriptor heap type as dictated by Direct3D 12 specs
enum class DescriptorHeapType {
    cbv_srv_uav,
    sampler,
    rtv,
    dsv
};

class DescriptorHeap final : public NamedEntity<class_names::D3D12DescriptorHeap>
{
    friend class Device;    // only devices are allowed to create the heaps
public:
    Device& device() const;    //! returns the device used to create this descriptor heap
    ComPtr<ID3D12DescriptorHeap> native() const;    //! returns encapsulated reference to the native Direct3D 12 interface representing the descriptor heap
    
    uint32_t getDescriptorSize() const;    //! returns size occupied in GPU memory by single descriptor

    DescriptorHeap(DescriptorHeap const&) = delete;
    DescriptorHeap(DescriptorHeap&&) = default;

    uint32_t capacity() const;


    /*! allocates multiple constant buffer view descriptors in the descriptor heap and
     returns GPU virtual address of the first allocated descriptor. This GPU address can
     afterwards be used to set the corresponding root descriptor table on the GPU side
     This function will fail if called on a descriptor heap of any type other than cbv_srv_uav

    */
    uint64_t allocateConstantBufferViewDescriptors(std::vector<CBVDescriptor> const& cbv_descriptors);


    /*! allocates multiple shader resource view descriptors in the descriptor heap and
     returns GPU virtual address of the first allocated descriptor. This GPU address can
     afterwards be used to set the corresponding root descriptor table on the GPU side
     This function will fail if called on a descriptor heap of any type other than cbv_srv_uav

    */
    uint64_t allocateShaderResourceViewDescriptors(std::vector<SRVDescriptor> const& srv_descriptors);

    /*! allocates multiple unordered access view descriptors in the descriptor heap and
     returns GPU virtual address of the first allocated descriptor. This GPU address can
     afterwards be used to set the corresponding root descriptor table on the GPU side
     This function will fail if called on a descriptor heap of any type other than cbv_srv_uav

    */
    uint64_t allocateUnorderedAccessViewDescriptors(std::vector<UAVDescriptor> const& uav_descriptors);
    
    /*! allocated multiple render target view descriptors in the descriptor heap and
     returns GPU virtual address of the first allocated descriptor. This GPU address can
     be afterwards used to set the corresponding root descriptor table on the GPU side.
     This function will fail if called on a descriptor heap of any type other than rtv
    */
    uint64_t allocateRenderTargetViewDescriptors(std::vector<RTVDescriptor> const& rtv_descriptors);

    /*! allocated multiple depth stencil view descriptors in the descriptor heap and
     returns GPU virtual address of the first allocated descriptor. This GPU address can
     be afterwards used to set the corresponding root descriptor table on the GPU side.
     This function will fail if called on a descriptor heap of any type other than dsv
    */
    uint64_t allocateDepthStencilViewDescriptors(std::vector<DSVDescriptor> const& dsv_descriptors);


private:
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> updateAllocation(uint32_t num_descriptors);

private:
    DescriptorHeap(Device& device, DescriptorHeapType type, uint32_t descriptor_capacity, uint32_t node_mask);


    Device& m_device;    //!< device that has created this heap
    ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;    //!< reference to the native Direct3D 12 descriptor heap interface
    DescriptorHeapType m_type;    //!< type of the descriptor heap
    uint32_t const m_descriptor_size;    //!< size of a single descriptor in the heap
    uint32_t const m_descriptor_capacity;    //!< number of descriptors that could be stored in the heap
    uint32_t m_node_mask;    //!< mask determining adapter, to which the heap is assigned
    std::atomic<uint32_t> m_num_descriptors_allocated;    //!< number of currently allocated descriptors
};

}

#endif
