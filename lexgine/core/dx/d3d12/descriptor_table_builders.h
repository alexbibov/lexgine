#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_BUILDERS_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_BUILDERS_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine_core_dx_d3d12_fwd.h"

#include <cstdint>
#include <vector>

namespace lexgine::core::dx::d3d12 {

struct ResourceDescriptorTableReference
{
    uint32_t page_id;
    uint32_t offset_from_heap_start;
    uint32_t descriptor_capacity;
};

class ResourceDescriptorTableBuilder final
{
public:
    ResourceDescriptorTableBuilder(Globals& globals);

    void addDescriptor(CBVDescriptor const& descriptor);
    void addDescriptor(SRVDescriptor const& descriptor);
    void addDescriptor(UAVDescriptor const& descriptor);
    
    ResourceDescriptorTableReference build() const;

private:
    enum class descriptor_cache_type
    {
        cbv, srv, uav, none
    };

    struct descriptor_range
    {
        descriptor_cache_type cache_type;
        size_t start;
        size_t end;
    };

    using table_footprint = std::vector<descriptor_range>;

    using cbv_descriptor_cache = std::vector<CBVDescriptor>;
    using srv_descriptor_cache = std::vector<SRVDescriptor>;
    using uav_descriptor_cache = std::vector<UAVDescriptor>;

private:
    Device const& m_current_device;
    GlobalSettings const& m_global_settings;
    DxResourceFactory const& m_dx_resource_factory;

    descriptor_cache_type m_currently_assembled_range;

    cbv_descriptor_cache m_cbv_descriptors;
    srv_descriptor_cache m_srv_descriptors;
    uav_descriptor_cache m_uav_descriptors;

    table_footprint m_descriptor_table_footprint;
};

}

#endif
