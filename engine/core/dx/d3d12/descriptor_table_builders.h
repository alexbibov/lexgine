#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_BUILDERS_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_BUILDERS_H

#include <cstdint>
#include <vector>

#include "engine/core/lexgine_core_fwd.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/srv_descriptor.h"
#include "engine/core/dx/d3d12/uav_descriptor.h"
#include "engine/core/dx/d3d12/cbv_descriptor.h"
#include "engine/core/dx/d3d12/sampler_descriptor.h"
#include "engine/core/dx/d3d12/rtv_descriptor.h"
#include "engine/core/dx/d3d12/dsv_descriptor.h"

namespace lexgine::core::dx::d3d12 {

class ResourceViewDescriptorTableBuilder final
{
public:
    ResourceViewDescriptorTableBuilder(Globals& globals, uint32_t target_descriptor_heap_page_id);

    void addDescriptor(CBVDescriptor const& descriptor);
    void addDescriptor(SRVDescriptor const& descriptor);
    void addDescriptor(UAVDescriptor const& descriptor);
    
    DescriptorTable build() const;

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
    Globals& m_globals;
    uint32_t m_target_descriptor_heap_page_id;

    descriptor_cache_type m_currently_assembled_range;

    cbv_descriptor_cache m_cbv_descriptors;
    srv_descriptor_cache m_srv_descriptors;
    uav_descriptor_cache m_uav_descriptors;

    table_footprint m_descriptor_table_footprint;
};



class SamplerDescriptorTableBuilder final
{
public:
    SamplerDescriptorTableBuilder(Globals& globals, uint32_t target_descriptor_heap_page);

    void addDescriptor(SamplerDescriptor const& descriptor);

    DescriptorTable build() const;

private:
    Globals& m_globals;
    uint32_t m_target_descriptor_heap_page_id;
    std::vector<SamplerDescriptor> m_sampler_descriptors;
};



class RenderTargetViewTableBuilder final
{
public:
    RenderTargetViewTableBuilder(Globals& globals, uint32_t target_descriptor_heap_page);
    void addDescriptor(RTVDescriptor const& descriptor);

    DescriptorTable build() const;

private:
    Globals& m_globals;
    uint32_t m_target_descriptor_heap_page_id;
    std::vector<RTVDescriptor> m_rtv_descriptors;
};



class DepthStencilViewTableBuilder final
{
public:
    DepthStencilViewTableBuilder(Globals& globals, uint32_t target_descriptor_heap_page);
    void addDescriptor(DSVDescriptor const& descriptor);

    DescriptorTable build() const;

private:
    Globals& m_globals;
    uint32_t m_target_descriptor_heap_page_id;
    std::vector<DSVDescriptor> m_dsv_descriptors;
};


}

#endif
