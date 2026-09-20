#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TARGET_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TARGET_H

#include <vector>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "descriptor_heap.h"
#include "engine/core/misc/optional.h"
#include "resource.h"
#include "resource_barrier_pack.h"
#include "descriptor_table_builders.h"
#include "rtv_descriptor.h"
#include "dsv_descriptor.h"


namespace lexgine::core::dx::d3d12 {

struct ColorTarget
{
    ResourceState target_default_state;
    RTVDescriptor target_view;

    ColorTarget(Resource const& target_resource, ResourceState target_default_state, RTVBufferInfo const& buffer_info);
    ColorTarget(Resource const& target_resource, ResourceState target_default_state, RTVTextureInfo const& texture_info);
    ColorTarget(Resource const& target_resource, ResourceState target_default_state, RTVTextureArrayInfo const& texture_array_info);
};

struct DepthTarget
{
    ResourceState target_default_state;
    DSVDescriptor target_view;

    DepthTarget(Resource const& target_resource, ResourceState target_default_state, DSVTextureInfo const& texture_info, DSVFlags flags = DSVFlags::none);
    DepthTarget(Resource const& target_resource, ResourceState target_default_state, DSVTextureArrayInfo const& texture_array_info, DSVFlags flags = DSVFlags::none);
};

class RenderingTarget
{
    //! Everything about an attached target that outlives the ColorTarget/DepthTarget the caller handed over
    struct TargetSnapshot
    {
        Resource const* p_resource;
        ResourceState default_state;
        uint16_t mipmap_level;
        uint64_t array_offset;
        uint32_t array_size;
    };

public:
    RenderingTarget(Globals& globals,
        std::vector<ColorTarget> const& color_targets, misc::Optional<DepthTarget> const& depth_target);

    /*! Repoints some or all of the attached targets at new resources, keeping the descriptor tables this
     rendering target already owns: the new views are written straight into the slots the existing tables
     occupy, so rtvTable() and dsvTable() stay valid and no caller has to rebind anything.

     @param new_color_targets replacements for the leading color targets, in the order the targets were
     attached at construction. Passing fewer than are attached updates only that leading run; passing an
     invalidated depth target likewise leaves the depth target alone.

     @return 'false' if the request named targets this rendering target does not have -- more color targets
     than are attached, or a depth target when none was attached at construction. Such a request is logged
     and its surplus dropped, while every target that could legally be updated still is.
    */
    bool updateRenderingTargets(
        std::vector<ColorTarget> const& new_color_targets,
        misc::Optional<DepthTarget> const& depth_target
    );

    void switchToRenderAccessState(CommandList const& command_list) const;
    void switchToInitialState(CommandList const& command_list) const;

    size_t count() const;    //! returns number of individual color targets
    bool hasDepth() const;    //! returns 'true' if this rendering target supports depth buffer; returns 'false' otherwise

    /*! Returns DXGI format of requested associated color rendering target. If color rendering target
     having requested index does not exist, returns DXGI_FORMAT_UNKNOWN
    */
    DXGI_FORMAT colorFormats(uint32_t index) const;

    /*! Returns DXGI format of the depth target associated with this rendering target
     If no depth targets were associated with this rendering target returns DXGI_FORMAT_UNKNOWN
    */
    DXGI_FORMAT depthFormat() const;

    DescriptorTable const& rtvTable() const;
    DescriptorTable const& dsvTable() const;

private:
    //! captures from a ColorTarget or a DepthTarget everything needed to re-derive its transition barriers
    template<typename Target>
    static TargetSnapshot makeSnapshot(Target const& target);

    void rebuildBarriers();    //!< re-derives both barrier packs from the currently attached targets

private:
    DynamicResourceBarrierPack m_forward_barriers;
    DynamicResourceBarrierPack m_backward_barriers;
    DescriptorTable m_rtvs_table;
    DescriptorTable m_dsv_table;

    std::vector<TargetSnapshot> m_color_targets;
    misc::Optional<TargetSnapshot> m_depth_target;

    std::vector<DXGI_FORMAT> m_color_target_formats;
    DXGI_FORMAT m_depth_target_format;
};


}

#endif
