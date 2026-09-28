#include "descriptor_table_builders.h"

#include "cbv_descriptor.h"
#include "srv_descriptor.h"
#include "uav_descriptor.h"
#include "sampler_descriptor.h"
#include "rtv_descriptor.h"
#include "dsv_descriptor.h"

#include <cassert>
#include <numeric>

namespace lexgine::core::dx::d3d12 {

void ResourceViewDescriptorTableBuilder::addDescriptor(CBVDescriptor const& descriptor)
{
    if (m_currently_assembled_range_type != ShaderVisibleMemoryResourceType::cbv)
    {
        descriptor_range new_range;
        new_range.resource_type = ShaderVisibleMemoryResourceType::cbv;
        new_range.start = m_cbv_descriptors.size();
        m_descriptor_table_footprint.push_back(new_range);

        m_currently_assembled_range_type = ShaderVisibleMemoryResourceType::cbv;
    }

    m_cbv_descriptors.push_back(descriptor);
    m_descriptor_table_footprint.back().end = m_cbv_descriptors.size();
}

void ResourceViewDescriptorTableBuilder::addDescriptor(SRVDescriptor const& descriptor)
{
    if (m_currently_assembled_range_type != ShaderVisibleMemoryResourceType::srv)
    {
        descriptor_range new_range;
        new_range.resource_type = ShaderVisibleMemoryResourceType::srv;
        new_range.start = m_srv_descriptors.size();
        m_descriptor_table_footprint.push_back(new_range);

        m_currently_assembled_range_type = ShaderVisibleMemoryResourceType::srv;
    }

    m_srv_descriptors.push_back(descriptor);
    m_descriptor_table_footprint.back().end = m_srv_descriptors.size();
}

void ResourceViewDescriptorTableBuilder::addDescriptor(UAVDescriptor const& descriptor)
{
    if (m_currently_assembled_range_type != ShaderVisibleMemoryResourceType::uav)
    {
        descriptor_range new_range;
        new_range.resource_type = ShaderVisibleMemoryResourceType::uav;
        new_range.start = m_uav_descriptors.size();
        m_descriptor_table_footprint.push_back(new_range);

        m_currently_assembled_range_type = ShaderVisibleMemoryResourceType::uav;
    }

    m_uav_descriptors.push_back(descriptor);
    m_descriptor_table_footprint.back().end = m_uav_descriptors.size();
}

DescriptorTable ResourceViewDescriptorTableBuilder::build(DescriptorAllocator& allocator) const
{
    DescriptorHeap& target_descriptor_heap = allocator.descriptorHeap();
    assert(target_descriptor_heap.type() == DescriptorHeapType::cbv_srv_uav);

    uint32_t total_descriptor_count = std::accumulate(m_descriptor_table_footprint.begin(),
        m_descriptor_table_footprint.end(), 0UI32,
        [](uint32_t a, descriptor_range const& range) -> uint32_t
        {
            return a + static_cast<uint32_t>(range.end - range.start);
        }
    );

    DescriptorTable rv = allocator.allocateDescriptorTable(total_descriptor_count);
    size_t offset = rv.offset;
    for (auto& range : m_descriptor_table_footprint)
    {
        switch (range.resource_type)
        {
        case ShaderVisibleMemoryResourceType::cbv:
            target_descriptor_heap.createConstantBufferViewDescriptors(offset,
                std::vector<CBVDescriptor>{m_cbv_descriptors.begin() + range.start,
                m_cbv_descriptors.begin() + range.end});
            offset += static_cast<uint32_t>(range.end - range.start);
            break;

        case ShaderVisibleMemoryResourceType::srv:
            target_descriptor_heap.createShaderResourceViewDescriptors(offset,
                std::vector<SRVDescriptor>{m_srv_descriptors.begin() + range.start,
                m_srv_descriptors.begin() + range.end});
            offset += static_cast<uint32_t>(range.end - range.start);
            break;

        case ShaderVisibleMemoryResourceType::uav:
            target_descriptor_heap.createUnorderedAccessViewDescriptors(offset,
                std::vector<UAVDescriptor>{m_uav_descriptors.begin() + range.start,
                m_uav_descriptors.begin() + range.end});
            offset += static_cast<uint32_t>(range.end - range.start);
            break;
        }
    }

    return rv;
}

void SamplerDescriptorTableBuilder::addDescriptor(SamplerDescriptor const& descriptor)
{
    m_sampler_descriptors.push_back(descriptor);
}

DescriptorTable SamplerDescriptorTableBuilder::build(PersistentDescriptorAllocator& allocator) const
{
    DescriptorHeap& target_descriptor_heap = allocator.descriptorHeap();
    assert(target_descriptor_heap.type() == DescriptorHeapType::sampler);

    DescriptorTable rv = allocator.allocateDescriptorTable(static_cast<uint32_t>(m_sampler_descriptors.size()));
    target_descriptor_heap.createSamplerDescriptors(rv.offset, m_sampler_descriptors);

    return rv;
}

void RenderTargetViewTableBuilder::addDescriptor(RTVDescriptor const& descriptor)
{
    m_rtv_descriptors.push_back(descriptor);
}

DescriptorTable RenderTargetViewTableBuilder::build(PersistentDescriptorAllocator& allocator) const
{
    DescriptorHeap& target_descriptor_heap = allocator.descriptorHeap();
    assert(target_descriptor_heap.type() == DescriptorHeapType::rtv);

    DescriptorTable rv = allocator.allocateDescriptorTable(static_cast<uint32_t>(m_rtv_descriptors.size()));
    target_descriptor_heap.createRenderTargetViewDescriptors(rv.offset, m_rtv_descriptors);

    return rv;
}

void DepthStencilViewTableBuilder::addDescriptor(DSVDescriptor const& descriptor)
{
    m_dsv_descriptors.push_back(descriptor);
}

DescriptorTable DepthStencilViewTableBuilder::build(PersistentDescriptorAllocator& allocator) const
{
    DescriptorHeap& target_descriptor_heap = allocator.descriptorHeap();
    assert(target_descriptor_heap.type() == DescriptorHeapType::dsv);

    DescriptorTable rv = allocator.allocateDescriptorTable(static_cast<uint32_t>(m_dsv_descriptors.size()));
    target_descriptor_heap.createDepthStencilViewDescriptors(rv.offset, m_dsv_descriptors);

    return rv;
}

}
