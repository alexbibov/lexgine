#include <algorithm>
#include <cassert>

#include "rendering_target.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;


ColorTarget::ColorTarget(Resource const& target_resource, ResourceState target_default_state, RTVBufferInfo const& buffer_info)
    : target_default_state{ target_default_state }
    , target_view{ target_resource, buffer_info }
{
}

ColorTarget::ColorTarget(Resource const& target_resource, ResourceState target_default_state, RTVTextureInfo const& texture_info)
    : target_default_state{ target_default_state }
    , target_view{ target_resource, texture_info }
{
}

ColorTarget::ColorTarget(Resource const& target_resource, ResourceState target_default_state, RTVTextureArrayInfo const& texture_array_info)
    : target_default_state{ target_default_state }
    , target_view{ target_resource, texture_array_info }
{
}

DepthTarget::DepthTarget(Resource const& target_resource, ResourceState target_default_state, DSVTextureInfo const& texture_info, DSVFlags flags/* = DSVFlags::none*/)
    : target_default_state{ target_default_state }
    , target_view{ target_resource, texture_info, flags }
{

}

DepthTarget::DepthTarget(Resource const& target_resource, ResourceState target_default_state, DSVTextureArrayInfo const& texture_array_info, DSVFlags flags/* = DSVFlags::none*/)
    : target_default_state{ target_default_state }
    , target_view{ target_resource, texture_array_info, flags }
{

}


RenderingTarget::RenderingTarget(Globals const& globals,
    std::vector<ColorTarget> const& color_targets, misc::Optional<DepthTarget> const& depth_target):
    m_has_depth{ depth_target.isValid() }
{
    RenderTargetViewTableBuilder rtv_table_builder{ globals, 0U };
    for (size_t i = 0U; i < color_targets.size(); ++i)
    {
        auto const& target = color_targets[i];
        auto viewed_array = target.target_view.viewedSubArray();
        assert(viewed_array.second == 1U);

        m_forward_barriers.addTransitionBarrier(&target.target_view.associatedResource(), 
            target.target_view.viewedMipmapLevel(), viewed_array.first,
            target.target_default_state, ResourceState::enum_type::render_target);

        m_backward_barriers.addTransitionBarrier(&target.target_view.associatedResource(), 
            target.target_view.viewedMipmapLevel(), viewed_array.first,
            ResourceState::enum_type::render_target, target.target_default_state);

        rtv_table_builder.addDescriptor(color_targets[i].target_view);
    }
    m_rtvs_table = rtv_table_builder.build();

    if (m_has_depth)
    {
        auto const& target = static_cast<DepthTarget const&>(depth_target);
        auto viewed_array = target.target_view.viewedSubArray();
        assert(viewed_array.second == 1U);

        m_forward_barriers.addTransitionBarrier(&target.target_view.associatedResource(), 
            target.target_view.viewedMipmapLevel(), viewed_array.first,
            target.target_default_state, ResourceState::enum_type::depth_write);

        m_backward_barriers.addTransitionBarrier(&target.target_view.associatedResource(),
            target.target_view.viewedMipmapLevel(), viewed_array.first,
            target.target_default_state, ResourceState::enum_type::depth_write);

        DepthStencilViewTableBuilder dsv_table_builder{ globals, 0U };
        dsv_table_builder.addDescriptor(target.target_view);
        m_dsv_table = dsv_table_builder.build();
    }
}

void RenderingTarget::switchToRenderAccessState(CommandList const& command_list) const
{
    m_forward_barriers.applyBarriers(command_list);
}

void RenderingTarget::switchToInitialState(CommandList const& command_list) const
{
    m_backward_barriers.applyBarriers(command_list);
}

size_t RenderingTarget::count() const
{
    return m_rtvs_table.descriptor_count;
}

bool RenderingTarget::hasDepth() const
{
    return m_has_depth;
}

RenderTargetViewDescriptorTable const& RenderingTarget::rtvTable() const
{
    return m_rtvs_table;
}

DepthStencilViewDescriptorTable const& RenderingTarget::dsvTable() const
{
    return m_dsv_table;
}

