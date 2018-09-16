#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_HEAP_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_HEAP_H

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"

#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

namespace lexgine {namespace core {namespace dx {namespace d3d12 {

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
    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle() const;   //! returns CPU pointer to the beginning of the heap
    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle() const;    //! returns GPU pointer to the beginning of the heap

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(uint32_t descriptor_id) const;   //! returns CPU pointer to the descriptor located in the heap, which corresponds to the zero-based index @param descriptor_id
    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(uint32_t descriptor_id) const;    //! returns GPU pointer to the descriptor located in the heap, which corresponds to the zero-based index @param descriptor_id

    uint32_t getDescriptorSize() const;    //! returns size occupied in GPU memory by single descriptor

    DescriptorHeap(DescriptorHeap const&) = delete;
    DescriptorHeap(DescriptorHeap&&) = default;

    uint32_t capacity() const;

private:
    DescriptorHeap(Device& device, DescriptorHeapType type, uint32_t num_descriptors, uint32_t node_mask);


    Device& m_device;    //!< device that has created this heap
    ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;    //!< reference to the native Direct3D 12 descriptor heap interface
    DescriptorHeapType m_type;    //!< type of the descriptor heap
    uint32_t m_descriptor_size;    //!< size of a single descriptor in the heap
    uint32_t m_num_descriptors;    //!< number of descriptors that could be stored in the heap
    uint32_t m_node_mask;    //!< mask determining adapter, to which the heap is assigned
};

}}}}

#endif
