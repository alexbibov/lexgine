#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/concurrency/abstract_task.h"

#include "rendering_loop.h"
#include "command_list.h"
#include "device.h"
#include "dx_resource_factory.h"
#include "descriptor_table_builders.h"

#include <cassert>

using namespace lexgine::core;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::dx::d3d12;


namespace {

class InitFrameTask : public AbstractTask
{
public:
    InitFrameTask(Device& device, DxResourceFactory const& dx_resources, math::Vector4f const& clear_color) :
        m_clear_color{ clear_color },
        m_command_list{ device.createCommandList(CommandType::direct, 0x1, FenceSharing::none) }
    {
        m_command_list.setStringName("clear_screen");

        for (int i = 0; i < 4U; ++i)
        {
            m_page0_descriptor_heaps[i] =
                &dx_resources.retrieveDescriptorHeap(device, static_cast<DescriptorHeapType>(i), 0);
        }
    }

private:    // required by the AbstractTask interface
    bool do_task(uint8_t worker_id, uint16_t frame_index) override
    {
        m_command_list.reset();

    }

    TaskType get_task_type() const override
    {
        return TaskType::gpu_draw;
    }

private:
    std::array<DescriptorHeap*, 4U> m_page0_descriptor_heaps;
    math::Vector4f m_clear_color;
    CommandList m_command_list;
};

}


RenderingLoopTarget::RenderingLoopTarget(Globals const& globals,
    std::vector<Resource> const& target_resources,
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
    m_rendering_tasks{ globals, {} }
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

