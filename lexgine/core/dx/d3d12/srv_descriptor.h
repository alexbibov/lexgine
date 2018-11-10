#ifndef LEXGINE_CORE_DX_D3D12_SRV_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_SRV_DESCRIPTOR_H

#include <d3d12.h>

#include <cinttypes>

#include "lexgine_core_dx_d3d12_fwd.h"


namespace lexgine::core::dx::d3d12 {


enum class SRVBufferInfoFlags
{
    none, raw
};

struct SRVBufferInfo final
{
    uint64_t first_element = 0ULL;
    uint32_t num_elements;
    uint32_t structure_byte_stride;
    SRVBufferInfoFlags flags = SRVBufferInfoFlags::none;
};


struct SRVTextureInfo final
{
    uint32_t most_detailed_mipmap_level = 0U;
    uint32_t num_mipmap_levels = 0xFFFFFFFF;
    float resource_min_lod_clamp = 0.f;
};


struct SRVTextureArrayInfo final
{
    uint32_t most_detailed_mipmap_level = 0U;
    uint32_t num_mipmap_levels = 0xFFFFFFFF;
    uint32_t first_array_element = 0U;
    uint32_t num_array_elements = 1U;
    float resource_min_lod_clamp = 0.f;
};


class SRVDescriptor final
{
public:
    SRVDescriptor(Resource const& resource, 
        SRVBufferInfo const& buffer_info);
    
    SRVDescriptor(Resource const& resource,
        SRVTextureInfo const& texture_info,
        bool is_cubemap = false);

    SRVDescriptor(Resource const& resource,
        SRVTextureArrayInfo const& texture_array_info,
        bool is_cubemap = false);

    /*! Normally, DXGI format for the SRV descriptor is fetched from resource descriptor.
     This function can be use if another setting for the format is preferable
    */
    void overrideFormat(DXGI_FORMAT format);

    D3D12_SHADER_RESOURCE_VIEW_DESC nativeDescriptor() const;
    Resource const& associatedResource() const;

private:
    Resource const& m_resource_ref;
    D3D12_SHADER_RESOURCE_VIEW_DESC m_native;
};

}

#endif
