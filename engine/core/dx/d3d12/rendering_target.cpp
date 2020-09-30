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


RenderingTarget::RenderingTarget(Globals& globals,
    std::vector<ColorTarget> const& color_targets, misc::Optional<DepthTarget> const& depth_target)
    : m_color_targets{ color_targets }
    , m_depth_target{ depth_target }
    , m_depth_target_format{ DXGI_FORMAT_UNKNOWN }
{
    RenderTargetViewTableBuilder rtv_table_builder{ globals, 0U };
    m_color_target_formats.resize(color_targets.size());
    for (size_t i = 0U; i < color_targets.size(); ++i)
    {
        auto const& target = color_targets[i];
        auto viewed_array = target.target_view.arrayOffsetAndSize();
        m_color_target_formats[i] = target.target_view.associatedResource().descriptor().format;

        rtv_table_builder.addDescriptor(target.target_view);
        for (size_t j = 0U; j < viewed_array.second; ++j)
        {
            m_forward_barriers.addTransitionBarrier(&target.target_view.associatedResource(),
                target.target_view.mipmapLevel(), static_cast<uint16_t>(viewed_array.first + j),
                target.target_default_state, ResourceState::base_values::render_target);

            m_backward_barriers.addTransitionBarrier(&target.target_view.associatedResource(),
                target.target_view.mipmapLevel(), static_cast<uint16_t>(viewed_array.first + j),
                ResourceState::base_values::render_target, target.target_default_state);
        }

        
    }
    m_rtvs_table = rtv_table_builder.build();


    if (m_depth_target.isValid())
    {
        auto const& target = static_cast<DepthTarget const&>(depth_target);
        m_depth_target_format = target.target_view.associatedResource().descriptor().format;

        DepthStencilViewTableBuilder dsv_table_builder{ globals, 0U };
        dsv_table_builder.addDescriptor(target.target_view);
        m_dsv_table = dsv_table_builder.build();

        auto viewed_array = target.target_view.arrayOffsetAndSize();
        for(size_t j = 0; j < viewed_array.second; ++j)
        {
            m_forward_barriers.addTransitionBarrier(&target.target_view.associatedResource(),
                target.target_view.mipmapLevel(), static_cast<uint16_t>(viewed_array.first + j),
                target.target_default_state, ResourceState::base_values::depth_write);

            m_backward_barriers.addTransitionBarrier(&target.target_view.associatedResource(),
                target.target_view.mipmapLevel(), static_cast<uint16_t>(viewed_array.first + j),
                ResourceState::base_values::depth_write, target.target_default_state);
        }
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
    return m_depth_target.isValid();
}

DXGI_FORMAT  RenderingTarget::colorFormats(uint32_t index) const
{
    return index < m_color_target_formats.size() ? m_color_target_formats[index] : DXGI_FORMAT_UNKNOWN;
}

DXGI_FORMAT RenderingTarget::depthFormat() const
{
    return m_depth_target_format;
}

RenderTargetViewDescriptorTable const& RenderingTarget::rtvTable() const
{
    return m_rtvs_table;
}

DepthStencilViewDescriptorTable const& RenderingTarget::dsvTable() const
{
    return m_dsv_table;
}