#include <cassert>
#include "rendering_target.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

RenderingTargetColor::RenderingTargetColor(Globals const& globals,
    std::vector<Resource> const& render_targets,
    std::vector<ResourceState> const& render_target_initial_states,
    std::vector<RTVDescriptor> const& render_target_resource_views,
    uint64_t active_color_targets) :
    m_active_color_targets{ active_color_targets }
{
    assert(render_targets.size() == render_target_initial_states.size()
        && render_target_initial_states.size() == render_target_resource_views.size());

    RenderTargetViewTableBuilder rtv_table_builder{ globals, 0U };
    for (size_t i = 0U; i < render_targets.size(); ++i)
    {
        m_forward_barriers.addTransitionBarrier(&render_targets[i], render_target_initial_states[i],
            ResourceState::enum_type::render_target);

        m_backward_barriers.addTransitionBarrier(&render_targets[i], ResourceState::enum_type::render_target,
            render_target_initial_states[i]);

        rtv_table_builder.addDescriptor(render_target_resource_views[i]);
    }
    m_rtvs_table = rtv_table_builder.build();
}

void RenderingTargetColor::switchToRenderAccessState(CommandList const& command_list) const
{
    m_forward_barriers.applyBarriers(command_list);
}

void RenderingTargetColor::switchToInitialState(CommandList const& command_list) const
{
    m_backward_barriers.applyBarriers(command_list);
}

void RenderingTargetColor::setActiveColorTargets(uint64_t active_color_targets_mask)
{
    m_active_color_targets = active_color_targets_mask;
}

uint64_t RenderingTargetColor::activeColorTargetsMask() const
{
    return m_active_color_targets;
}

size_t RenderingTargetColor::totalTargetsCount() const
{
    return m_rtvs_table.descriptor_count;
}

size_t RenderingTargetColor::activeTargetsCount() const
{
    return misc::getSetBitCount(m_active_color_targets);
}

RenderTargetViewDescriptorTable const& RenderingTargetColor::rtvTable() const
{
    return m_rtvs_table;
}



RenderingTargetDepth::RenderingTargetDepth(Globals const& globals,
    Resource const& depth_target_resource,
    ResourceState initial_depth_target_resource_state,
    DSVDescriptor const& depth_target_resource_view)
{
    DepthStencilViewTableBuilder dsv_table_builder{ globals, 0U };
    dsv_table_builder.addDescriptor(depth_target_resource_view);
    m_dsv_table = dsv_table_builder.build();

    m_forward_barriers.addTransitionBarrier(&depth_target_resource, initial_depth_target_resource_state,
        ResourceState::enum_type::depth_write);
    m_backward_barriers.addTransitionBarrier(&depth_target_resource, ResourceState::enum_type::depth_write,
        initial_depth_target_resource_state);
}

void RenderingTargetDepth::switchToRenderAccessState(CommandList const& command_list) const
{
    m_forward_barriers.applyBarriers(command_list);
}

void RenderingTargetDepth::switchToInitialState(CommandList const& command_list) const
{
    m_backward_barriers.applyBarriers(command_list);
}

DepthStencilViewDescriptorTable const& RenderingTargetDepth::dsvTable() const
{
    return m_dsv_table;
}