#include "descriptor_heap.h"
#include "device.h"
#include "../../exception.h"


using namespace lexgine::core::dx::d3d12;

Device& lexgine::core::dx::d3d12::DescriptorHeap::device() const
{
    return m_device;
}

ComPtr<ID3D12DescriptorHeap> DescriptorHeap::native() const
{
    return m_descriptor_heap;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::getCPUHandle() const
{
    return m_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::getGPUHandle() const
{
    return m_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::getCPUHandle(uint32_t descriptor_id) const
{
    return D3D12_CPU_DESCRIPTOR_HANDLE{ getCPUHandle().ptr + descriptor_id*m_descriptor_size };
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::getGPUHandle(uint32_t descriptor_id) const
{
    return D3D12_GPU_DESCRIPTOR_HANDLE{ getGPUHandle().ptr + descriptor_id*m_descriptor_size };
}

uint32_t DescriptorHeap::getDescriptorSize() const
{
    return m_device.native()->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(m_type));
}

DescriptorHeap::DescriptorHeap(Device& device, DescriptorHeapType type, uint32_t num_descriptors, uint32_t node_mask) :
    m_device{ device },
    m_type{ type },
    m_descriptor_size{ device.native()->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(type)) },
    m_num_descriptors{ num_descriptors },
    m_node_mask{ node_mask }
{
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(type);
    desc.NumDescriptors = num_descriptors;
    switch (type)
    {
    case DescriptorHeapType::cbv_srv_uav:
    case DescriptorHeapType::sampler:
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        break;

    case DescriptorHeapType::rtv:
    case DescriptorHeapType::dsv:
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    }
    desc.NodeMask = node_mask;

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        device.native()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descriptor_heap)),
        S_OK
    );
}