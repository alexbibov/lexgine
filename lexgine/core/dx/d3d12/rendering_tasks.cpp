#include "rendering_tasks.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/logging_streams.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/math/vector_types.h"

#include "dx_resource_factory.h"
#include "device.h"
#include "command_list.h"
#include "rendering_target.h"
#include "profiler.h"


using namespace lexgine::core;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::dx::d3d12;


namespace {

std::vector<std::ostream*> convertFileStreamsToGenericStreams(std::vector<std::ofstream>& fstreams)
{
    std::vector<std::ostream*> res(fstreams.size());
    std::transform(fstreams.begin(), fstreams.end(), res.begin(), [](std::ofstream& fs)->std::ostream* { return &fs; });
    return res;
}

}


class RenderingTasks::FrameBeginTask final : public RootSchedulableTask
{
public:
    FrameBeginTask(RenderingTasks& rendering_tasks,
        math::Vector4f const& clear_color) :
        RootSchedulableTask{ "frame_begin_task" },
        m_rendering_tasks{ rendering_tasks },
        m_clear_color{ clear_color },
        m_command_list{ rendering_tasks.m_device.createCommandList(CommandType::direct, 0x1) }
    {
        m_command_list.setStringName("clear_screen");

        m_page0_descriptor_heaps.resize(2U);    // we only fetch two heaps: cbv_srv_uav and sampler since only these two must be set from the command list
        for (int i = 0; i < 2U; ++i)
        {
            m_page0_descriptor_heaps[i] =
                &m_rendering_tasks.m_dx_resources.retrieveDescriptorHeap(rendering_tasks.m_device, static_cast<DescriptorHeapType>(i), 0);
        }
    }

    void setClearColor(math::Vector4f const& color)
    {
        m_clear_color = color;
    }

private:    // required by the AbstractTask interface
    bool do_task(uint8_t worker_id, uint16_t frame_index) override
    {
        uint64_t current_frame_index = m_rendering_tasks.m_end_of_frame_cpu_wall.lastValueSignaled();

        if (current_frame_index > m_rendering_tasks.m_frame_consumed_wall.lastValueSignaled()
            + m_rendering_tasks.m_queued_frames_count)
        {
            YieldProcessor();
            return false;    // we return 'false' here because we want this task to be rescheduled at a later time
        }

        PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
        "CPU job for frame %i start", current_frame_index);

        uint16_t frame_id = current_frame_index % m_rendering_tasks.m_queued_frames_count;
        auto& target = m_rendering_tasks.m_targets[frame_id];

        m_command_list.reset();
        m_command_list.setDescriptorHeaps(m_page0_descriptor_heaps);
        m_command_list.inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle);

        m_command_list.outputMergerSetRenderTargets(&target->rtvTable(), 1,
            target->hasDepth() ? &target->dsvTable() : nullptr, 0U);

        m_rendering_tasks.m_targets[frame_id]->switchToRenderAccessState(m_command_list);

        m_command_list.clearRenderTargetView(target->rtvTable(), 0, m_clear_color);

        m_command_list.close();

        m_rendering_tasks.m_device.defaultCommandQueue().executeCommandList(m_command_list);

        return true;
    }

    TaskType get_task_type() const override
    {
        return TaskType::gpu_draw;
    }

private:
    RenderingTasks& m_rendering_tasks;
    misc::StaticVector<DescriptorHeap const*, 4U> m_page0_descriptor_heaps;
    math::Vector4f m_clear_color;
    CommandList m_command_list;
};

class RenderingTasks::FrameEndTask final : public SchedulableTask
{
    public:
    FrameEndTask(RenderingTasks& rendering_tasks) :
        SchedulableTask{ "frame_end_task" },
        m_rendering_tasks{ rendering_tasks },
        m_command_list{ m_rendering_tasks.m_device.createCommandList(CommandType::direct, 0x1) }
    {

    }

private:    // required by the AbstractTask interface
    bool do_task(uint8_t worker_id, uint16_t frame_index) override
    {
        uint64_t current_frame_index = m_rendering_tasks.m_end_of_frame_cpu_wall.lastValueSignaled();
        uint16_t frame_id = current_frame_index % m_rendering_tasks.m_queued_frames_count;
        auto& target = m_rendering_tasks.m_targets[frame_id];

        m_command_list.reset();

        target->switchToInitialState(m_command_list);

        m_command_list.close();
        m_rendering_tasks.m_device.defaultCommandQueue().executeCommandList(m_command_list);
        m_rendering_tasks.m_end_of_frame_gpu_wall.signalFromGPU(m_rendering_tasks.m_device.defaultCommandQueue());
        
        PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
            "CPU job for frame %i finish", current_frame_index);

        m_rendering_tasks.m_end_of_frame_cpu_wall.signalFromCPU();

        return true;
    }

    TaskType get_task_type() const override
    {
        return TaskType::gpu_draw;
    }

private:
    RenderingTasks& m_rendering_tasks;
    CommandList m_command_list;
};


RenderingTasks::RenderingTasks(Globals& globals):
    m_dx_resources{ *globals.get<DxResourceFactory>() },
    m_device{ *globals.get<Device>() },
    m_task_graph{ globals.get<GlobalSettings>()->getNumberOfWorkers(), "RenderingTasksGraph" },
    m_task_sink{ m_task_graph, convertFileStreamsToGenericStreams(globals.get<LoggingStreams>()->worker_logging_streams), "RenderingTasksSink" },

    m_frame_begin_task{ new FrameBeginTask{*this, math::Vector4f{0.f, 0.f, 0.f, 0.f}} },
    m_frame_end_task{ new FrameEndTask{*this} },

    m_queued_frames_count{ globals.get<GlobalSettings>()->getMaxFramesInFlight() },
    m_end_of_frame_cpu_wall{ m_device },
    m_end_of_frame_gpu_wall{ m_device },
    m_frame_consumed_wall{ m_device }
{
    m_task_graph.setRootNodes({ m_frame_begin_task.get() });
    m_frame_begin_task->addDependent(*m_frame_end_task);
}

RenderingTasks::~RenderingTasks() = default;


void RenderingTasks::run()
{
    m_task_sink.run();
}

void RenderingTasks::dispatchExitSignal()
{
    m_task_sink.dispatchExitSignal();
}

void RenderingTasks::setRenderingTargets(std::vector<std::shared_ptr<RenderingTarget>> const& multiframe_targets)
{
    if (multiframe_targets.size() != m_queued_frames_count)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
            "Unable to set rendering targets for the rendering tasks: the number of targets being set must be"
            " equal to the maximal length of frame buffer queue (" + std::to_string(m_queued_frames_count)
            + " frames as defined by the active settings). However, the number of targets provided was " + std::to_string(multiframe_targets.size()));
    }

    m_targets = multiframe_targets;
}

void RenderingTasks::consumeFrame()
{
    m_frame_consumed_wall.signalFromCPU();
}

void RenderingTasks::waitUntilFrameIsReady(uint64_t frame_index) const
{
    m_end_of_frame_gpu_wall.waitUntilValue(frame_index);
}

uint64_t RenderingTasks::totalFramesScheduled() const
{
    return m_end_of_frame_cpu_wall.lastValueSignaled();
}

uint64_t RenderingTasks::totalFramesRendered() const
{
    return m_end_of_frame_gpu_wall.lastValueSignaled();
}

uint64_t RenderingTasks::totalFramesConsumed() const
{
    return m_frame_consumed_wall.lastValueSignaled();
}

