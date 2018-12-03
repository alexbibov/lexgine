#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/concurrency/schedulable_task.h"

#include "rendering_loop.h"
#include "command_list.h"
#include "device.h"
#include "dx_resource_factory.h"
#include "descriptor_table_builders.h"
#include "rtv_descriptor.h"
#include "dsv_descriptor.h"

#include <cassert>

using namespace lexgine::core;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::dx::d3d12;


class RenderingLoop::InitFrameTask : public RootSchedulableTask
{
public:
    InitFrameTask(RenderingLoop& rendering_loop, 
        math::Vector4f const& clear_color) :
        RootSchedulableTask{ "begin_frame_task" },
        m_rendering_loop{ rendering_loop },
        m_clear_color{ clear_color },
        m_command_list{ rendering_loop.m_device.createCommandList(CommandType::direct, 0x1) }
    {
        m_command_list.setStringName("clear_screen");

        m_page0_descriptor_heaps.resize(4U);
        for (int i = 0; i < 4U; ++i)
        {
            m_page0_descriptor_heaps[i] =
                &m_rendering_loop.m_dx_resources.retrieveDescriptorHeap(rendering_loop.m_device, static_cast<DescriptorHeapType>(i), 0);
        }
    }

    void setClearColor(math::Vector4f const& color)
    {
        m_clear_color = color;
    }

private:    // required by the AbstractTask interface
    bool do_task(uint8_t worker_id, uint16_t frame_index) override
    {
        m_command_list.reset();
        m_command_list.setDescriptorHeaps(m_page0_descriptor_heaps);
        m_command_list.inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle);
        m_command_list.outputMergerSetRenderTargets(
            m_rendering_loop.m_color_target_ptr
            ? &m_rendering_loop.m_color_target_ptr->rtvTable()
            : nullptr,
            m_rendering_loop.m_depth_target_ptr
            ? &m_rendering_loop.m_depth_target_ptr->dsvTable()
            : nullptr,
            0U);
        m_rendering_loop.m_color_target_ptr->switchToRenderAccessState(m_command_list);
        m_rendering_loop.m_depth_target_ptr->switchToRenderAccessState(m_command_list);

        for(size_t i = 0; i < m_rendering_loop.m_color_target_ptr->count(); ++i)
            m_command_list.clearRenderTargetView(m_rendering_loop.m_color_target_ptr->rtvTable(), 
                static_cast<uint32_t>(i), m_clear_color);

        m_command_list.close();

        m_rendering_loop.m_device.defaultCommandQueue().executeCommandList(m_command_list);

        return true;
    }

    TaskType get_task_type() const override
    {
        return TaskType::other;
    }

private:
    RenderingLoop& m_rendering_loop;
    std::vector<DescriptorHeap const*> m_page0_descriptor_heaps;
    math::Vector4f m_clear_color;
    CommandList m_command_list;
};

class RenderingLoop::PostFrameTask : public SchedulableTask
{
public:
    PostFrameTask(RenderingLoop& rendering_loop) :
        SchedulableTask{ "end_frame_task" },
        m_rendering_loop{ rendering_loop },
        m_command_list{ m_rendering_loop.m_device.createCommandList(CommandType::direct, 0x1) }
    {

    }

private:    // required by the AbstractTask interface
    bool do_task(uint8_t worker_id, uint16_t frame_index) override
    {
        return true;
    }

    TaskType get_task_type() const override
    {
        return TaskType::other;
    }

private:
    RenderingLoop& m_rendering_loop;
    CommandList m_command_list;
};


RenderingLoopColorTarget::RenderingLoopColorTarget(Globals const& globals,
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

void RenderingLoopColorTarget::switchToRenderAccessState(CommandList const& command_list) const
{
    m_forward_barriers.applyBarriers(command_list);
}

void RenderingLoopColorTarget::switchToInitialState(CommandList const& command_list) const
{
    m_backward_barriers.applyBarriers(command_list);
}

size_t RenderingLoopColorTarget::count() const
{
    return m_rtvs_table.descriptor_count;
}

RenderTargetViewDescriptorTable const& RenderingLoopColorTarget::rtvTable() const
{
    return m_rtvs_table;
}



RenderingLoopDepthTarget::RenderingLoopDepthTarget(Globals const& globals, 
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

void RenderingLoopDepthTarget::switchToRenderAccessState(CommandList const& command_list) const
{
    m_forward_barriers.applyBarriers(command_list);
}

void RenderingLoopDepthTarget::switchToInitialState(CommandList const& command_list) const
{
    m_backward_barriers.applyBarriers(command_list);
}

DepthStencilViewDescriptorTable const& RenderingLoopDepthTarget::dsvTable() const
{
    return m_dsv_table;
}


RenderingLoop::RenderingLoop(Globals& globals,
    std::shared_ptr<RenderingLoopColorTarget> const& rendering_loop_color_target_ptr,
    std::shared_ptr<RenderingLoopDepthTarget> const& rendering_loop_depth_target_ptr) :
    m_device{ *globals.get<Device>() },
    m_dx_resources{ *globals.get<DxResourceFactory>() },
    m_global_settings{ *globals.get<GlobalSettings>() },
    m_queued_frame_counter{ 0U },
    m_color_target_ptr{ rendering_loop_color_target_ptr },
    m_depth_target_ptr{ rendering_loop_depth_target_ptr },
    m_init_frame_task_ptr{ new InitFrameTask{ *this, math::Vector4f{0.f, 0.f, 0.f, 0.f} } },
    m_post_frame_task_ptr{ new PostFrameTask{*this} },
    m_rendering_tasks{ globals, {m_init_frame_task_ptr.get()}, m_post_frame_task_ptr.get() }
{
    
}

RenderingLoop::~RenderingLoop() = default;

void RenderingLoop::draw()
{
    PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
        "CPU job for frame %i start", m_queued_frame_counter);

    m_rendering_tasks.run();
   
    PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
        "CPU job for frame %i finish", m_queued_frame_counter);

    m_queued_frame_counter = (m_queued_frame_counter + 1) % m_global_settings.getMaxFramesInFlight();
}

void RenderingLoop::setFrameBackgroundColor(math::Vector4f const& color)
{
    m_init_frame_task_ptr->setClearColor(color);
}


