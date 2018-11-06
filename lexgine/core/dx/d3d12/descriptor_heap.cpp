#include "descriptor_heap.h"
#include "device.h"
#include "lexgine/core/exception.h"

#include "resource.h"
#include "constant_buffer_view_descriptor.h"
#include "shader_resource_view_descriptor.h"
#include "unordered_access_view_descriptor.h"
#include "render_target_view_descriptor.h"
#include "depth_stencil_view_descriptor.h"


#include <cassert>

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
    return m_device.native()->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(m_type));
}

uint32_t DescriptorHeap::capacity() const
{
    return m_descriptor_capacity;
}

uint64_t DescriptorHeap::allocateConstantBufferViewDescriptors(std::vector<ConstantBufferViewDescriptor> const& cbv_descriptors)
{
    assert(m_type == DescriptorHeapType::cbv_srv_uav);

    auto ptrs = updateAllocation(static_cast<uint32_t>(cbv_descriptors.size()));

    for (auto const& cbv_desc : cbv_descriptors)
        m_device.native()->CreateConstantBufferView(&cbv_desc.nativeDescriptor(), ptrs.first);

    return ptrs.second.ptr;
}

uint64_t DescriptorHeap::allocateShaderResourceViewDescriptors(std::vector<ShaderResourceViewDescriptor> const& srv_descriptors)
{
    assert(m_type == DescriptorHeapType::cbv_srv_uav);

    auto ptrs = updateAllocation(static_cast<uint32_t>(srv_descriptors.size()));
    
    for (auto const& srv_desc : srv_descriptors)
        m_device.native()->CreateShaderResourceView(srv_desc.associatedResource().native().Get(), &srv_desc.nativeDescriptor(), ptrs.first);

    return ptrs.second.ptr;
}

uint64_t DescriptorHeap::allocateUnorderedAccessViewDescriptors(std::vector<UnorderedAccessViewDescriptor> const& uav_descriptors)
{
    assert(m_type == DescriptorHeapType::cbv_srv_uav);

    auto ptrs = updateAllocation(static_cast<uint32_t>(uav_descriptors.size()));

    for (auto const& uav_desc : uav_descriptors)
        m_device.native()->CreateUnorderedAccessView(uav_desc.associatedResource().native().Get(),
            uav_desc.associatedCounterResourcePtr()->native().Get(), &uav_desc.nativeDescriptor(), ptrs.first);

    return ptrs.second.ptr;
}

uint64_t DescriptorHeap::allocateRenderTargetViewDescriptors(std::vector<RenderTargetViewDescriptor> const& rtv_descriptors)
{
    assert(m_type == DescriptorHeapType::rtv);

    auto ptrs = updateAllocation(static_cast<uint32_t>(rtv_descriptors.size()));

    for (auto const& rtv_desc : rtv_descriptors)
        m_device.native()->CreateRenderTargetView(rtv_desc.associatedResource().native().Get(), &rtv_desc.nativeDescriptor(), ptrs.first);

    return ptrs.second.ptr;
}

uint64_t DescriptorHeap::allocateDepthStencilViewDescriptors(std::vector<DepthStencilViewDescriptor> const & dsv_descriptors)
{
    assert(m_type == DescriptorHeapType::dsv);

    auto ptrs = updateAllocation(static_cast<uint32_t>(dsv_descriptors.size()));

    for (auto const& dsv_desc : dsv_descriptors)
        m_device.native()->CreateDepthStencilView(dsv_desc.associatedResource().native().Get(), &dsv_desc.nativeDescriptor(), ptrs.first);

    return ptrs.second.ptr;
}

std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> DescriptorHeap::updateAllocation(uint32_t num_descriptors)
{
    uint32_t num_descriptors_allocated_old = m_num_descriptors_allocated.fetch_add(num_descriptors, std::memory_order_release);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle{ m_descriptor_heap->GetCPUDescriptorHandleForHeapStart() };
    cpu_descriptor_handle.ptr += m_descriptor_size * num_descriptors_allocated_old;

    D3D12_GPU_DESCRIPTOR_HANDLE gpu_descriptor_handle{ m_descriptor_heap->GetGPUDescriptorHandleForHeapStart() };
    gpu_descriptor_handle.ptr += m_descriptor_size * num_descriptors_allocated_old;

    return std::make_pair(cpu_descriptor_handle, gpu_descriptor_handle);
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
}
