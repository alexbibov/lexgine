#ifndef LEXGINE_CORE_DX_D3D12_UAV_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_UAV_DESCRIPTOR_H

#include <d3d12.h>
#include <cinttypes>

#include "lexgine_core_dx_d3d12_fwd.h"

namespace lexgine::core::dx::d3d12 {

enum class UnorderedAccessViewBufferInfoFlags
{
    none, raw
};

struct UAVBufferInfo final
{
    uint64_t first_element = 0ULL;
    uint32_t num_elements;
    uint32_t structure_byte_stride;
    uint64_t counter_offset_in_bytes;
    UnorderedAccessViewBufferInfoFlags flags = UnorderedAccessViewBufferInfoFlags::none;
};


struct UAVTextureInfo final
{
    uint32_t mip_level_slice = 0U;

    uint32_t _3d_texture_first_layer = 0U;    //!< first layer of 3D texture to be accessible through the UAV (ignored for 1D and 2D textures)
    uint32_t _3d_texture_num_layers = 0xFFFFFFFF;    //! < number of layers of 3D texture to be accessible through the UAV (ignored for 1D and 2D textures)
};


struct UAVTextureArrayInfo final
{
    uint32_t mip_level_slice = 0U;
    uint32_t first_array_element = 0U;
    uint32_t num_array_elements = 1U;
};


class UAVDescriptor final
{
public:
    UAVDescriptor(PlacedResource const& resource,
        UAVBufferInfo const& buffer_info,
        PlacedResource const* p_counter_resource = nullptr);

    UAVDescriptor(PlacedResource const& resource,
        UAVTextureInfo const& texture_info,
        PlacedResource const* p_counter_resource = nullptr);

    UAVDescriptor(PlacedResource const& resource,
        UAVTextureArrayInfo const& texture_array_info,
        PlacedResource const* p_counter_resource = nullptr);

    /*! Normally, DXGI format for the SRV descriptor is fetched from resource descriptor.
     This function can be use if another setting for the format is preferable
    */
    void overrideFormat(DXGI_FORMAT format);

    D3D12_UNORDERED_ACCESS_VIEW_DESC nativeDescriptor() const;
    PlacedResource const& associatedResource() const;
    PlacedResource const* associatedCounterResourcePtr() const;

private:
    PlacedResource const& m_resource_ref;
    PlacedResource const* m_counter_resource_ptr;
    D3D12_UNORDERED_ACCESS_VIEW_DESC m_native;
};

}

#endif
