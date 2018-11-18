#include "descriptor_table_builders.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/dx/d3d12/dx_resource_factory.h"

#include "device.h"
#include "cbv_descriptor.h"
#include "srv_descriptor.h"
#include "uav_descriptor.h"
#include "sampler_descriptor.h"
#include "rtv_descriptor.h"
#include "dsv_descriptor.h"

#include <numeric>

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

ResourceViewDescriptorTableBuilder::ResourceViewDescriptorTableBuilder(Globals const& globals, uint32_t target_descriptor_heap_page_id):
    m_globals{ globals },
    m_target_descriptor_heap_page_id{ target_descriptor_heap_page_id },
    m_currently_assembled_range{ descriptor_cache_type::none }
{

}

void ResourceViewDescriptorTableBuilder::addDescriptor(CBVDescriptor const& descriptor)
{
    if (m_currently_assembled_range != descriptor_cache_type::cbv)
    {
        switch (m_currently_assembled_range)
        {
        case descriptor_cache_type::srv:
            m_descriptor_table_footprint.back().end = m_srv_descriptors.size();
            break;
        case descriptor_cache_type::uav:
            m_descriptor_table_footprint.back().end = m_uav_descriptors.size();
            break;
        }

        descriptor_range new_range;
        new_range.cache_type = descriptor_cache_type::cbv;
        new_range.start = m_cbv_descriptors.size();
        m_descriptor_table_footprint.push_back(new_range);

        m_currently_assembled_range = descriptor_cache_type::cbv;
    }

    m_cbv_descriptors.push_back(descriptor);
}

void ResourceViewDescriptorTableBuilder::addDescriptor(SRVDescriptor const& descriptor)
{
    if (m_currently_assembled_range != descriptor_cache_type::srv)
    {
        switch (m_currently_assembled_range)
        {
        case descriptor_cache_type::cbv:
            m_descriptor_table_footprint.back().end = m_cbv_descriptors.size();
            break;
        case descriptor_cache_type::uav:
            m_descriptor_table_footprint.back().end = m_uav_descriptors.size();
            break;
        }

        descriptor_range new_range;
        new_range.cache_type = descriptor_cache_type::srv;
        new_range.start = m_srv_descriptors.size();
        m_descriptor_table_footprint.push_back(new_range);

        m_currently_assembled_range = descriptor_cache_type::srv;
    }

    m_srv_descriptors.push_back(descriptor);
}

void ResourceViewDescriptorTableBuilder::addDescriptor(UAVDescriptor const& descriptor)
{
    if (m_currently_assembled_range != descriptor_cache_type::uav)
    {
        switch (m_currently_assembled_range)
        {
        case descriptor_cache_type::cbv:
            m_descriptor_table_footprint.back().end = m_cbv_descriptors.size();
            break;
        case descriptor_cache_type::srv:
            m_descriptor_table_footprint.back().end = m_srv_descriptors.size();
            break;
        }

        descriptor_range new_range;
        new_range.cache_type = descriptor_cache_type::uav;
        new_range.start = m_uav_descriptors.size();
        m_descriptor_table_footprint.push_back(new_range);

        m_currently_assembled_range = descriptor_cache_type::uav;
    }

    m_uav_descriptors.push_back(descriptor);
}

ResourceViewDescriptorTableReference ResourceViewDescriptorTableBuilder::build() const
{
    uint32_t total_descriptor_count = std::accumulate(m_descriptor_table_footprint.begin(),
        m_descriptor_table_footprint.end(), 0UI32,
        [](uint32_t a, descriptor_range const& range) -> uint32_t
        {
            return a + static_cast<uint32_t>(range.end - range.start);
        }
    );
    

    auto& target_descriptor_heap = 
        m_globals.get<DxResourceFactory>()->retrieveDescriptorHeap(*m_globals.get<Device>(), 
            DescriptorHeapType::cbv_srv_uav, m_target_descriptor_heap_page_id);

    
    uint32_t offset = target_descriptor_heap.reserveDescriptors(total_descriptor_count);
    ResourceViewDescriptorTableReference rv{ m_target_descriptor_heap_page_id, offset, total_descriptor_count };
    for (auto& range : m_descriptor_table_footprint)
    {
        switch (range.cache_type)
        {
        case descriptor_cache_type::cbv:
            target_descriptor_heap.createConstantBufferViewDescriptors(offset,
                std::vector<CBVDescriptor>{m_cbv_descriptors.begin() + range.start,
                m_cbv_descriptors.begin() + range.end});
            offset += static_cast<uint32_t>(range.end - range.start);
            break;

        case descriptor_cache_type::srv:
            target_descriptor_heap.createShaderResourceViewDescriptors(offset,
                std::vector<SRVDescriptor>{m_srv_descriptors.begin() + range.start,
                m_srv_descriptors.begin() + range.end});
            offset += static_cast<uint32_t>(range.end - range.start);
            break;

        case descriptor_cache_type::uav:
            target_descriptor_heap.createUnorderedAccessViewDescriptors(offset,
                std::vector<UAVDescriptor>{m_uav_descriptors.begin() + range.start,
                m_uav_descriptors.begin() + range.end});
            offset += static_cast<uint32_t>(range.end - range.start);
            break;
        }
    }
    
    return rv;
}

SamplerTableBuilder::SamplerTableBuilder(Globals const& globals, uint32_t target_descriptor_heap_page):
    m_globals{ globals },
    m_target_descriptor_heap_page_id{ target_descriptor_heap_page }
{
}

void SamplerTableBuilder::addDescriptor(SamplerDescriptor const& descriptor)
{
    m_sampler_descriptors.push_back(descriptor);
}

SamplerDescriptorTableReference SamplerTableBuilder::build() const
{
    auto& target_descriptor_heap = m_globals.get<DxResourceFactory>()->retrieveDescriptorHeap(
        *m_globals.get<Device>(), DescriptorHeapType::sampler, m_target_descriptor_heap_page_id);

    uint32_t offset = target_descriptor_heap.reserveDescriptors(static_cast<uint32_t>(m_sampler_descriptors.size()));
    SamplerDescriptorTableReference rv{ m_target_descriptor_heap_page_id, offset, static_cast<uint32_t>(m_sampler_descriptors.size()) };

    target_descriptor_heap.createSamplerDescriptors(offset, m_sampler_descriptors);

    return rv;
}

