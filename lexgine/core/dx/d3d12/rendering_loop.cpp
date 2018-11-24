#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"

#include "rendering_loop.h"
#include "command_list.h"
#include "device.h"

#include <cassert>

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;


RenderingLoopTarget::RenderingLoopTarget(std::vector<Resource> const& target_resources,
    std::vector<ResourceState> const& target_resources_initial_states):
    m_target_resources{ target_resources }
{
    assert(target_resources.size() == target_resources_initial_states.size());

    for (size_t i = 0U; i < target_resources.size(); ++i)
    {
        m_forward_barriers.addTransitionBarrier(&target_resources[i], target_resources_initial_states[i],
            ResourceState::enum_type::render_target);

        m_backward_barriers.addTransitionBarrier(&target_resources[i], ResourceState::enum_type::render_target,
            target_resources_initial_states[i]);
    }
}

void RenderingLoopTarget::switchToRenderAccessState(CommandList const& command_list) const
{
    m_forward_barriers.applyBarriers(command_list);
}

void RenderingLoopTarget::switchToInitialState(CommandList const& command_list) const
{
    m_backward_barriers.applyBarriers(command_list);
}



RenderingLoop::RenderingLoop(Globals const& globals,
    std::shared_ptr<RenderingLoopTarget> const& rendering_loop_target_ptr):
    m_device{ *globals.get<Device>() },
    m_global_settings{ *globals.get<GlobalSettings>() },
    m_queued_frame_counter{ 0U },
    m_rendering_loop_target_ptr{ rendering_loop_target_ptr },
    m_rendering_tasks{ globals }
{
}

void RenderingLoop::draw()
{
    PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
        "CPU job for frame %i start", m_queued_frame_counter);



   
    PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
        "CPU job for frame %i finish", m_queued_frame_counter);

    m_queued_frame_counter = (m_queued_frame_counter + 1) % m_global_settings.getMaxFramesInFlight();
}

