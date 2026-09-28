#ifndef LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_BUILDERS_H
#define LEXGINE_CORE_DX_D3D12_DESCRIPTOR_TABLE_BUILDERS_H

#include <cstdint>
#include <vector>

#include "engine/core/lexgine_core_fwd.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/descriptor_allocator.h"
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
    ResourceViewDescriptorTableBuilder() = default;

    void addDescriptor(CBVDescriptor const& descriptor);
    void addDescriptor(SRVDescriptor const& descriptor);
    void addDescriptor(UAVDescriptor const& descriptor);

    DescriptorTable build(DescriptorAllocator& allocator) const;    //! allocates the table using allocator serving cbv_srv_uav descriptor heap

private:

    struct descriptor_range
    {
        ShaderVisibleMemoryResourceType resource_type;
        size_t start;
        size_t end;
    };

private:
    ShaderVisibleMemoryResourceType m_currently_assembled_range_type{ ShaderVisibleMemoryResourceType::count };

    std::vector<CBVDescriptor> m_cbv_descriptors;
    std::vector<SRVDescriptor> m_srv_descriptors;
    std::vector<UAVDescriptor> m_uav_descriptors;

    std::vector<descriptor_range> m_descriptor_table_footprint;
};



class SamplerDescriptorTableBuilder final
{
public:
    void addDescriptor(SamplerDescriptor const& descriptor);

    DescriptorTable build(PersistentDescriptorAllocator& allocator) const;    //! allocates the table using allocator serving sampler descriptor heap

private:
    std::vector<SamplerDescriptor> m_sampler_descriptors;
};



class RenderTargetViewTableBuilder final
{
public:
    void addDescriptor(RTVDescriptor const& descriptor);

    DescriptorTable build(PersistentDescriptorAllocator& allocator) const;    //! allocates the table using allocator serving rtv descriptor heap

private:
    std::vector<RTVDescriptor> m_rtv_descriptors;
};



class DepthStencilViewTableBuilder final
{
public:
    void addDescriptor(DSVDescriptor const& descriptor);

    DescriptorTable build(PersistentDescriptorAllocator& allocator) const;    //! allocates the table using allocator serving dsv descriptor heap

private:
    std::vector<DSVDescriptor> m_dsv_descriptors;
};


}

#endif
