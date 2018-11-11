#include "descriptor_table_builders.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/dx/d3d12/dx_resource_factory.h"

#include "device.h"
#include "cbv_descriptor.h"
#include "srv_descriptor.h"
#include "uav_descriptor.h"

#include <numeric>

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

ResourceDescriptorTableBuilder::ResourceDescriptorTableBuilder(Globals& globals):
    m_current_device{ *globals.get<Device>() },
    m_global_settings{ *globals.get<GlobalSettings>() },
    m_dx_resource_factory{ *globals.get<DxResourceFactory>() },
    m_currently_assembled_range{ descriptor_cache_type::none }
{

}

void ResourceDescriptorTableBuilder::addDescriptor(CBVDescriptor const& descriptor)
{
    if (m_currently_assembled_range != ResourceDescriptorTableBuilder::descriptor_cache_type::cbv)
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

        m_currently_assembled_range = ResourceDescriptorTableBuilder::descriptor_cache_type::cbv;
    }

    m_cbv_descriptors.push_back(descriptor);
}

void ResourceDescriptorTableBuilder::addDescriptor(SRVDescriptor const& descriptor)
{
    if (m_currently_assembled_range != ResourceDescriptorTableBuilder::descriptor_cache_type::srv)
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

        m_currently_assembled_range = ResourceDescriptorTableBuilder::descriptor_cache_type::srv;
    }

    m_srv_descriptors.push_back(descriptor);
}

void ResourceDescriptorTableBuilder::addDescriptor(UAVDescriptor const& descriptor)
{
    if (m_currently_assembled_range != ResourceDescriptorTableBuilder::descriptor_cache_type::uav)
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

        m_currently_assembled_range = ResourceDescriptorTableBuilder::descriptor_cache_type::uav;
    }

    m_uav_descriptors.push_back(descriptor);
}

ResourceDescriptorTableReference ResourceDescriptorTableBuilder::build() const
{
    uint32_t total_descriptor_count = std::accumulate(m_descriptor_table_footprint.begin(),
        m_descriptor_table_footprint.end(), 0UI32,
        [](uint32_t a, descriptor_range const& range) -> uint32_t
        {
            return a + static_cast<uint32_t>(range.end - range.start);
        }
    );

    ResourceDescriptorTableReference rv{};

    

    return rv;
}
