#include <algorithm>
#include <cassert>

#include "rendering_target.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

RenderingTargetColor::RenderingTargetColor(Globals const& globals, std::vector<TargetDescriptor> const& targets,
    std::vector<RTVDescriptor> const& views)
{
    assert(targets.size() == views.size());

    RenderTargetViewTableBuilder rtv_table_builder{ globals, 0U };
    for (size_t i = 0U; i < targets.size(); ++i)
    {
        TargetDescriptor const& desc = targets[i];
        m_forward_barriers.addTransitionBarrier(&desc.target_resource, desc.target_mipmap_level, desc.target_array_layer,
            desc.target_initial_state, ResourceState::enum_type::render_target);

        m_backward_barriers.addTransitionBarrier(&desc.target_resource, desc.target_mipmap_level, desc.target_array_layer,
            ResourceState::enum_type::render_target, desc.target_initial_state);

        rtv_table_builder.addDescriptor(views[i]);
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



RenderingTargetDepth::RenderingTargetDepth(Globals const& globals, std::vector<TargetDescriptor> const& targets,
    std::vector<DSVDescriptor> const& views)
{
    assert(targets.size() == views.size());

    DepthStencilViewTableBuilder dsv_table_builder{ globals, 0U };
    for (size_t i = 0U; i < targets.size(); ++i)
    {
        TargetDescriptor const& desc = targets[i];

        m_forward_barriers.addTransitionBarrier(&desc.target_resource, desc.target_mipmap_level, desc.target_array_layer,
            desc.target_initial_state, ResourceState::enum_type::depth_write);

        m_backward_barriers.addTransitionBarrier(&desc.target_resource, desc.target_mipmap_level, desc.target_array_layer,
            ResourceState::enum_type::depth_write, desc.target_initial_state);

        dsv_table_builder.addDescriptor(views[i]);
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