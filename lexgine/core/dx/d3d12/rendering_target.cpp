#include <cassert>
#include "rendering_target.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

RenderingTargetColor::RenderingTargetColor(Globals const& globals,
    std::vector<Resource> const& render_targets,
    std::vector<ResourceState> const& render_target_initial_states,
    std::vector<RTVDescriptor> const& render_target_resource_views)
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

size_t RenderingTargetColor::targetCount() const
{
    return m_rtvs_table.descriptor_count;
}

RenderTargetViewDescriptorTable const& RenderingTargetColor::rtvTable() const
{
    return m_rtvs_table;
}



RenderingTargetDepth::RenderingTargetDepth(Globals const& globals,
    std::vector<Resource> const& depth_targets,
    std::vector<ResourceState> const& depth_target_initial_states,
    std::vector<DSVDescriptor> const& depth_target_resource_views)
{
    assert(depth_targets.size() == depth_target_initial_states.size()
        && depth_target_initial_states.size() == depth_target_resource_views.size());

    DepthStencilViewTableBuilder dsv_table_builder{ globals, 0U };

    for (size_t i = 0U; i < depth_targets.size(); ++i)
    {
        m_forward_barriers.addTransitionBarrier(&depth_targets[i], depth_target_initial_states[i],
            ResourceState::enum_type::depth_write);
        m_backward_barriers.addTransitionBarrier(&depth_targets[i], ResourceState::enum_type::depth_write,
            depth_target_initial_states[i]);

        dsv_table_builder.addDescriptor(depth_target_resource_views[i]);
    }

    m_dsv_table = dsv_table_builder.build();

    
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