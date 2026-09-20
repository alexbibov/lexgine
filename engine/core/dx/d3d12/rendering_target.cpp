#include <algorithm>
#include <cassert>
#include <string>

#include "engine/core/misc/log.h"

#include "rendering_target.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;


namespace
{
    void logRenderingTargetError(std::string const& message)
    {
        if (misc::Log* p_logger = misc::Log::retrieve())
        {
            p_logger->out("ERROR: RenderingTarget update rejected: " + message, misc::LogMessageType::error);
        }
    }
}  // namespace


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


template<typename Target>
RenderingTarget::TargetSnapshot RenderingTarget::makeSnapshot(Target const& target)
{
    auto viewed_array = target.target_view.arrayOffsetAndSize();
    return {
        &target.target_view.associatedResource(),
        target.target_default_state,
        static_cast<uint16_t>(target.target_view.mipmapLevel()),
        viewed_array.first,
        viewed_array.second
    };
}


RenderingTarget::RenderingTarget(Globals& globals,
    std::vector<ColorTarget> const& color_targets, misc::Optional<DepthTarget> const& depth_target)
    : m_depth_target_format{ DXGI_FORMAT_UNKNOWN }
{
    RenderTargetViewTableBuilder rtv_table_builder{ globals };
    m_color_target_formats.resize(color_targets.size());
    m_color_targets.reserve(color_targets.size());
    for (size_t i = 0U; i < color_targets.size(); ++i)
    {
        auto const& target = color_targets[i];
        m_color_target_formats[i] = target.target_view.associatedResource().descriptor().format;
        m_color_targets.push_back(makeSnapshot(target));

        rtv_table_builder.addDescriptor(target.target_view);
    }
    m_rtvs_table = rtv_table_builder.build();


    if (depth_target.isValid())
    {
        auto const& target = *depth_target;
        m_depth_target_format = target.target_view.associatedResource().descriptor().format;
        m_depth_target = makeSnapshot(target);

        DepthStencilViewTableBuilder dsv_table_builder{ globals };
        dsv_table_builder.addDescriptor(target.target_view);
        m_dsv_table = dsv_table_builder.build();
    }

    rebuildBarriers();
}

void RenderingTarget::rebuildBarriers()
{
    m_forward_barriers.clear();
    m_backward_barriers.clear();

    for (auto const& target : m_color_targets)
    {
        for (uint32_t j = 0U; j < target.array_size; ++j)
        {
            m_forward_barriers.addTransitionBarrier(target.p_resource,
                target.mipmap_level, static_cast<uint16_t>(target.array_offset + j),
                target.default_state, ResourceState::base_values::render_target);

            m_backward_barriers.addTransitionBarrier(target.p_resource,
                target.mipmap_level, static_cast<uint16_t>(target.array_offset + j),
                ResourceState::base_values::render_target, target.default_state);
        }
    }

    if (m_depth_target.isValid())
    {
        auto const& target = *m_depth_target;
        for (uint32_t j = 0U; j < target.array_size; ++j)
        {
            m_forward_barriers.addTransitionBarrier(target.p_resource,
                target.mipmap_level, static_cast<uint16_t>(target.array_offset + j),
                target.default_state, ResourceState::base_values::depth_write);

            m_backward_barriers.addTransitionBarrier(target.p_resource,
                target.mipmap_level, static_cast<uint16_t>(target.array_offset + j),
                ResourceState::base_values::depth_write, target.default_state);
        }
    }
}

bool RenderingTarget::updateRenderingTargets(
    std::vector<ColorTarget> const& new_color_targets,
    misc::Optional<DepthTarget> const& depth_target)
{
    bool rv = true;

    size_t updated_color_target_count = new_color_targets.size();
    if (updated_color_target_count > m_color_targets.size())
    {
        logRenderingTargetError("requested update of " + std::to_string(updated_color_target_count)
            + " color targets, but only " + std::to_string(m_color_targets.size()) + " are attached");
        updated_color_target_count = m_color_targets.size();
        rv = false;
    }

    bool update_depth_target = depth_target.isValid();
    if (update_depth_target && !hasDepth())
    {
        logRenderingTargetError("requested update of the depth target, but none was attached on construction");
        update_depth_target = false;
        rv = false;
    }

    for (size_t i = 0U; i < updated_color_target_count; ++i)
    {
        auto const& target = new_color_targets[i];
        m_rtvs_table.p_heap->createRenderTargetViewDescriptor(m_rtvs_table.offset + i, target.target_view);
        m_color_target_formats[i] = target.target_view.associatedResource().descriptor().format;
        m_color_targets[i] = makeSnapshot(target);
    }

    if (update_depth_target)
    {
        auto const& target = *depth_target;
        m_dsv_table.p_heap->createDepthStencilViewDescriptor(m_dsv_table.offset, target.target_view);
        m_depth_target_format = target.target_view.associatedResource().descriptor().format;
        m_depth_target = makeSnapshot(target);
    }

    if (updated_color_target_count || update_depth_target) rebuildBarriers();

    return rv;
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

DescriptorTable const& RenderingTarget::rtvTable() const
{
    return m_rtvs_table;
}

DescriptorTable const& RenderingTarget::dsvTable() const
{
    return m_dsv_table;
}