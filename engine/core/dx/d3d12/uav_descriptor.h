#ifndef LEXGINE_CORE_DX_D3D12_UAV_DESCRIPTOR_H
#define LEXGINE_CORE_DX_D3D12_UAV_DESCRIPTOR_H

#include <cstdint>
#include <utility>

#include <d3d12.h>

#include "lexgine_core_dx_d3d12_fwd.h"
#include "hashable_descriptor.h"

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


class UAVDescriptor final : public HashableDescriptor<D3D12_UNORDERED_ACCESS_VIEW_DESC>
{
public:
    UAVDescriptor(Resource const& resource,
        UAVBufferInfo const& buffer_info,
        Resource const* p_counter_resource = nullptr);

    UAVDescriptor(Resource const& resource,
        UAVTextureInfo const& texture_info,
        Resource const* p_counter_resource = nullptr);

    UAVDescriptor(Resource const& resource,
        UAVTextureArrayInfo const& texture_array_info,
        Resource const* p_counter_resource = nullptr);

    /*! Normally, DXGI format for the UAV descriptor is fetched from resource descriptor.
     This function can be use if another setting for the format is preferable
    */
    void overrideFormat(DXGI_FORMAT format);

    D3D12_UNORDERED_ACCESS_VIEW_DESC nativeDescriptor() const;
    Resource const& associatedResource() const;
    Resource const* associatedCounterResourcePtr() const;

    uint32_t mipmapLevel() const;    //! returns mipmap level attached to the view

    //! for compatible resources returns the first array slice and the total number of array elements that were attached to the view
    std::pair<uint64_t, uint32_t> arrayOffsetAndSize() const;

private:
    Resource const& m_resource_ref;
    Resource const* m_counter_resource_ptr;
    D3D12_UNORDERED_ACCESS_VIEW_DESC m_native;
};

}

#endif
