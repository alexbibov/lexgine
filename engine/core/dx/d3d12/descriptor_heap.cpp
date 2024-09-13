#include "descriptor_heap.h"
#include "device.h"
#include "engine/core/exception.h"
#include "engine/core/dx/d3d12/d3d12_tools.h"

#include "resource.h"
#include "cbv_descriptor.h"
#include "srv_descriptor.h"
#include "uav_descriptor.h"
#include "sampler_descriptor.h"
#include "rtv_descriptor.h"
#include "dsv_descriptor.h"

#include <cassert>
#include <algorithm>


namespace lexgine::core::dx::d3d12 {

Device& DescriptorHeap::device() const
{
    return m_device;
}

ComPtr<ID3D12DescriptorHeap> DescriptorHeap::native() const
{
    return m_descriptor_heap;
}

uint32_t DescriptorHeap::getDescriptorSize() const
{
    return m_descriptor_size;
}

size_t DescriptorHeap::getBaseCPUPointer() const
{
    return m_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr;
}

uint64_t DescriptorHeap::getBaseGPUPointer() const
{
    return m_is_gpu_visible ? m_descriptor_heap->GetGPUDescriptorHandleForHeapStart().ptr : 0;
}

void DescriptorHeap::reset()
{
    m_num_descriptors_allocated.store(0, std::memory_order_release);
}

uint32_t DescriptorHeap::capacity() const
{
    return m_descriptor_capacity;
}

uint32_t DescriptorHeap::descriptorsAllocated() const
{
    return (std::min)(m_num_descriptors_allocated.load(std::memory_order::memory_order_acquire), m_descriptor_capacity);
}

uint32_t DescriptorHeap::reserveDescriptors(uint32_t count)
{
    uint32_t offset;
    if (count <= m_descriptor_capacity - descriptorsAllocated())
    {
        offset = m_num_descriptors_allocated.fetch_add(count, std::memory_order::memory_order_acq_rel);
    }
    else
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(*this,
            "Unable to reserve " + std::to_string(count) + " descriptors from descriptor heap \""
            + getStringName() + "\": the descriptor heap is exhausted");
    }

    return offset;
}

uint64_t DescriptorHeap::createConstantBufferViewDescriptor(size_t offset, CBVDescriptor const& cbv_descriptor)
{
    assert(m_type == DescriptorHeapType::cbv_srv_uav);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle { m_heap_start_cpu_address + offset * m_descriptor_size };
    auto cbv_desc_native = cbv_descriptor.nativeDescriptor();
    m_device.native()->CreateConstantBufferView(&cbv_desc_native, cpu_handle);

    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createConstantBufferViewDescriptors(size_t offset, std::vector<CBVDescriptor> const& cbv_descriptors)
{
    size_t current_offset = offset;
    for (auto const& cbv_desc : cbv_descriptors) 
    {
        createConstantBufferViewDescriptor(current_offset++, cbv_desc);
    }
    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createShaderResourceViewDescriptor(size_t offset, SRVDescriptor const& srv_descriptor)
{
    assert(m_type == DescriptorHeapType::cbv_srv_uav);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle{ m_heap_start_cpu_address + offset * m_descriptor_size };
    auto srv_desc_native = srv_descriptor.nativeDescriptor();
    m_device.native()->CreateShaderResourceView(srv_descriptor.associatedResource().native().Get(), &srv_desc_native, cpu_handle);

    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createShaderResourceViewDescriptors(size_t offset, std::vector<SRVDescriptor> const& srv_descriptors)
{
    size_t current_offset = offset;
    for (auto const& srv_desc : srv_descriptors) 
    {
        createShaderResourceViewDescriptor(current_offset++, srv_desc);
    }
    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createUnorderedAccessViewDescriptor(size_t offset, UAVDescriptor const& uav_descriptor)
{
    assert(m_type == DescriptorHeapType::cbv_srv_uav);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle { m_heap_start_cpu_address + offset * m_descriptor_size };

    auto p_counter_resource = uav_descriptor.associatedCounterResourcePtr();

    auto uav_desc_native = uav_descriptor.nativeDescriptor();
    m_device.native()->CreateUnorderedAccessView(uav_descriptor.associatedResource().native().Get(),
        p_counter_resource ? p_counter_resource->native().Get() : nullptr,
        &uav_desc_native, cpu_handle);

    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createUnorderedAccessViewDescriptors(size_t offset, std::vector<UAVDescriptor> const& uav_descriptors)
{
    size_t current_offset = offset;
    for (auto const& uav_desc : uav_descriptors) 
    {
        createUnorderedAccessViewDescriptor(current_offset++, uav_desc);
    }
    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createSamplerDescriptor(size_t offset, SamplerDescriptor const& sampler_descriptor)
{
    assert(m_type == DescriptorHeapType::sampler);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle { m_heap_start_cpu_address + offset * m_descriptor_size };
    auto sampler_desc_native = sampler_descriptor.nativeDescriptor();
    m_device.native()->CreateSampler(&sampler_desc_native, cpu_handle);
    
    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createSamplerDescriptors(size_t offset, std::vector<SamplerDescriptor> const& sampler_descriptors)
{
    size_t current_offset = offset;
    for (auto const& sampler_desc : sampler_descriptors) 
    {
        createSamplerDescriptor(current_offset++, sampler_desc);
    }
    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createRenderTargetViewDescriptor(size_t offset, RTVDescriptor const& rtv_descriptor)
{
    assert(m_type == DescriptorHeapType::rtv);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle { m_heap_start_cpu_address + offset * m_descriptor_size };
    auto rtv_desc_native = rtv_descriptor.nativeDescriptor();
    m_device.native()->CreateRenderTargetView(rtv_descriptor.associatedResource().native().Get(), &rtv_desc_native, cpu_handle);

    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createRenderTargetViewDescriptors(size_t offset, std::vector<RTVDescriptor> const& rtv_descriptors)
{
    size_t current_offset = offset;
    for (auto const& rtv_desc : rtv_descriptors)
    {
        createRenderTargetViewDescriptor(current_offset++, rtv_desc);
    }
    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createDepthStencilViewDescriptor(size_t offset, DSVDescriptor const& dsv_descriptor)
{
    assert(m_type == DescriptorHeapType::dsv);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle { m_heap_start_cpu_address + offset * m_descriptor_size };
    auto dsv_desc_native = dsv_descriptor.nativeDescriptor();
    m_device.native()->CreateDepthStencilView(dsv_descriptor.associatedResource().native().Get(), &dsv_desc_native, cpu_handle);

    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createDepthStencilViewDescriptors(size_t offset, std::vector<DSVDescriptor> const& dsv_descriptors)
{
    size_t current_offset = offset;
    for (auto const& dsv_desc : dsv_descriptors) 
    {
        createDepthStencilViewDescriptor(current_offset++, dsv_desc);
    }
    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

DescriptorHeap::DescriptorHeap(Device& device, DescriptorHeapType type, uint32_t descriptor_capacity, uint32_t node_mask) :
    m_device{ device },
    m_type{ type },
    m_descriptor_size{ device.native()->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(type)) },
    m_descriptor_capacity{ descriptor_capacity },
    m_node_mask{ node_mask },
    m_num_descriptors_allocated{ 0U }
{
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(type);
    desc.NumDescriptors = static_cast<UINT>(descriptor_capacity);
    switch (type)
    {
    case DescriptorHeapType::cbv_srv_uav:
    case DescriptorHeapType::sampler:
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        m_is_gpu_visible = true;
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

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = m_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    m_heap_start_cpu_address = cpu_handle.ptr;
    m_heap_start_gpu_address = 0;

    if (m_is_gpu_visible)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = m_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
        m_heap_start_gpu_address = gpu_handle.ptr;
    }
}

}