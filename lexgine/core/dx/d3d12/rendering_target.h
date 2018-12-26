#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TARGET_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TARGET_H

#include <vector>

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "resource.h"
#include "resource_barrier_pack.h"
#include "descriptor_table_builders.h"
#include "rtv_descriptor.h"
#include "dsv_descriptor.h"


namespace lexgine::core::dx::d3d12 {

struct TargetDescriptor
{
    Resource target_resource;
    uint32_t target_mipmap_level = 0U;
    uint32_t target_array_layer = 0U;
    ResourceState target_initial_state;
};

class RenderingTargetColor
{
public:
    RenderingTargetColor(Globals const& globals, std::vector<TargetDescriptor> const& targets,
        std::vector<RTVDescriptor> const& views);

    void switchToRenderAccessState(CommandList const& command_list) const;
    void switchToInitialState(CommandList const& command_list) const;

    size_t targetCount() const;    //! returns total number of color targets

    RenderTargetViewDescriptorTable const& rtvTable() const;

private:
    DynamicResourceBarrierPack m_forward_barriers;
    DynamicResourceBarrierPack m_backward_barriers;
    RenderTargetViewDescriptorTable m_rtvs_table;
};


class RenderingTargetDepth
{
public:
    RenderingTargetDepth(Globals const& globals, std::vector<TargetDescriptor> const& targets,
        std::vector<DSVDescriptor> const& views);

    void switchToRenderAccessState(CommandList const& command_list) const;
    void switchToInitialState(CommandList const& command_list) const;

    DepthStencilViewDescriptorTable const& dsvTable() const;

private:
    DynamicResourceBarrierPack m_forward_barriers;
    DynamicResourceBarrierPack m_backward_barriers;
    DepthStencilViewDescriptorTable m_dsv_table;
};


}

#endif
