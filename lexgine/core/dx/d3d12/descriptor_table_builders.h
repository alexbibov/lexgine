#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_BUILDERS_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_BUILDERS_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine_core_dx_d3d12_fwd.h"

#include <cstdint>
#include <vector>

namespace lexgine::core::dx::d3d12 {

template<typename T>
struct TableReference final
{
    uint32_t page_id;
    uint32_t offset_from_heap_start;
    uint32_t descriptor_capacity;
};

struct tag_CBV_SRV_UAV;
using ResourceViewDescriptorTableReference = TableReference<tag_CBV_SRV_UAV>;

class ResourceViewDescriptorTableBuilder final
{
public:
    ResourceViewDescriptorTableBuilder(Globals const& globals, uint32_t target_descriptor_heap_page_id);

    void addDescriptor(CBVDescriptor const& descriptor);
    void addDescriptor(SRVDescriptor const& descriptor);
    void addDescriptor(UAVDescriptor const& descriptor);
    
    ResourceViewDescriptorTableReference build() const;

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
    Globals const& m_globals;
    uint32_t m_target_descriptor_heap_page_id;

    descriptor_cache_type m_currently_assembled_range;

    cbv_descriptor_cache m_cbv_descriptors;
    srv_descriptor_cache m_srv_descriptors;
    uav_descriptor_cache m_uav_descriptors;

    table_footprint m_descriptor_table_footprint;
};


struct tag_Sampler;
using SamplerDescriptorTableReference = TableReference<tag_Sampler>;

class SamplerTableBuilder final
{
public:
    SamplerTableBuilder(Globals const& globals, uint32_t target_descriptor_heap_page);

    void addDescriptor(SamplerDescriptor const& descriptor);

    SamplerDescriptorTableReference build() const;

private:
    Globals const& m_globals;
    uint32_t m_target_descriptor_heap_page_id;
    std::vector<SamplerDescriptor> m_sampler_descriptors;
};


struct tag_RTV;
using RenderTargetViewDescriptorTableReference = TableReference<tag_RTV>;

class RenderTargetViewTableBuilder final
{
public:
    RenderTargetViewTableBuilder(Globals const& globals, uint32_t target_descriptor_heap_page);
    void addDescriptor(RTVDescriptor const& descriptor);

    RenderTargetViewDescriptorTableReference build() const;

private:
    Globals const& m_globals;
    uint32_t m_target_descriptor_heap_page_id;
    std::vector<RTVDescriptor> m_rtv_descriptors;
};


struct tag_DSV;
using DepthStencilViewDescriptorTableReference = TableReference<tag_DSV>;

class DepthStencilViewTableBuilder final
{
public:
    DepthStencilViewTableBuilder(Globals const& globals, uint32_t target_descriptor_heap_page);
    void addDescriptor(DSVDescriptor const& descriptor);

    DepthStencilViewDescriptorTableReference build() const;

private:
    Globals const& m_globals;
    uint32_t m_target_descriptor_heap_page_id;
    std::vector<DSVDescriptor> m_dsv_descriptors;
};


}

#endif
