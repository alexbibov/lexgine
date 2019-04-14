#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TARGET_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TARGET_H

#include <vector>

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/misc/optional.h"
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
public:
    RenderingTarget(Globals& globals, 
        std::vector<ColorTarget> const& color_targets, misc::Optional<DepthTarget> const& depth_target);

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

    RenderTargetViewDescriptorTable const& rtvTable() const;
    DepthStencilViewDescriptorTable const& dsvTable() const;

private:
    DynamicResourceBarrierPack m_forward_barriers;
    DynamicResourceBarrierPack m_backward_barriers;
    RenderTargetViewDescriptorTable m_rtvs_table;
    DepthStencilViewDescriptorTable m_dsv_table;

    std::vector<ColorTarget> m_color_targets;
    misc::Optional<DepthTarget> m_depth_target;

    std::vector<DXGI_FORMAT> m_color_target_formats;
    DXGI_FORMAT m_depth_target_format;
};


}

#endif
