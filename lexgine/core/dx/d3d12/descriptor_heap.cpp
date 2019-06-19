#include "descriptor_heap.h"
#include "device.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/dx/d3d12/d3d12_tools.h"

#include "resource.h"
#include "cbv_descriptor.h"
#include "srv_descriptor.h"
#include "uav_descriptor.h"
#include "sampler_descriptor.h"
#include "rtv_descriptor.h"
#include "dsv_descriptor.h"

#include <cassert>
#include <algorithm>


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

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
    return m_descriptor_heap->GetGPUDescriptorHandleForHeapStart().ptr;
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
    bool reservation_successful = count <= m_descriptor_capacity - descriptorsAllocated();

    if (reservation_successful)
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


uint64_t DescriptorHeap::createConstantBufferViewDescriptors(size_t offset, std::vector<CBVDescriptor> const& cbv_descriptors)
{
    assert(m_type == DescriptorHeapType::cbv_srv_uav);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle{ m_heap_start_cpu_address + offset * m_descriptor_size };
    for (auto const& cbv_desc : cbv_descriptors)
    {
        m_device.native()->CreateConstantBufferView(&cbv_desc.nativeDescriptor(), cpu_handle);
        cpu_handle.ptr += m_descriptor_size;
    }

    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createShaderResourceViewDescriptors(size_t offset, std::vector<SRVDescriptor> const& srv_descriptors)
{
    assert(m_type == DescriptorHeapType::cbv_srv_uav);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle{ m_heap_start_cpu_address + offset * m_descriptor_size };
    for (auto const& srv_desc : srv_descriptors)
    {
        m_device.native()->CreateShaderResourceView(srv_desc.associatedResource().native().Get(), &srv_desc.nativeDescriptor(), cpu_handle);
        cpu_handle.ptr += m_descriptor_size;
    }

    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createUnorderedAccessViewDescriptors(size_t offset, std::vector<UAVDescriptor> const& uav_descriptors)
{
    assert(m_type == DescriptorHeapType::cbv_srv_uav);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle{ m_heap_start_cpu_address + offset * m_descriptor_size };
    for (auto const& uav_desc : uav_descriptors)
    {
        auto p_counter_resource = uav_desc.associatedCounterResourcePtr();

        m_device.native()->CreateUnorderedAccessView(uav_desc.associatedResource().native().Get(),
            p_counter_resource ? p_counter_resource->native().Get() : nullptr,
            &uav_desc.nativeDescriptor(), cpu_handle);
        cpu_handle.ptr += m_descriptor_size;
    }

    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createSamplerDescriptors(size_t offset, std::vector<SamplerDescriptor> const& sampler_descriptors)
{
    assert(m_type == DescriptorHeapType::sampler);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle{ m_heap_start_cpu_address + offset * m_descriptor_size };
    for (auto const& sampler_desc : sampler_descriptors)
    {
        m_device.native()->CreateSampler(&sampler_desc.nativeDescriptor(), cpu_handle);
        cpu_handle.ptr += m_descriptor_size;
    }

    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createRenderTargetViewDescriptors(size_t offset, std::vector<RTVDescriptor> const& rtv_descriptors)
{
    assert(m_type == DescriptorHeapType::rtv);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle{ m_heap_start_cpu_address + offset * m_descriptor_size };
    for (auto const& rtv_desc : rtv_descriptors)
    {
        m_device.native()->CreateRenderTargetView(rtv_desc.associatedResource().native().Get(), &rtv_desc.nativeDescriptor(), cpu_handle);
        cpu_handle.ptr += m_descriptor_size;
    }

    return m_heap_start_gpu_address + offset * m_descriptor_size;
}

uint64_t DescriptorHeap::createDepthStencilViewDescriptors(size_t offset, std::vector<DSVDescriptor> const& dsv_descriptors)
{
    assert(m_type == DescriptorHeapType::dsv);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle{ m_heap_start_cpu_address + offset * m_descriptor_size };
    for (auto const& dsv_desc : dsv_descriptors)
    {
        m_device.native()->CreateDepthStencilView(dsv_desc.associatedResource().native().Get(), &dsv_desc.nativeDescriptor(), cpu_handle);
        cpu_handle.ptr += m_descriptor_size;
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
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = m_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
    m_heap_start_cpu_address = cpu_handle.ptr;
    m_heap_start_gpu_address = gpu_handle.ptr;
}