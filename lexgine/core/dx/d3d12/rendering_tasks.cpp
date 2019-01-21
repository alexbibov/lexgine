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
    bool doTask(uint8_t worker_id, uint64_t current_frame_index) override
    {
        PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
        "CPU job for frame %i start", current_frame_index);

        auto& target = *m_rendering_tasks.m_current_rendering_target_ptr;

        m_command_list.reset();
        m_command_list.setDescriptorHeaps(m_page0_descriptor_heaps);
        m_command_list.inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle);

        m_command_list.outputMergerSetRenderTargets(&target.rtvTable(), 1,
            target.hasDepth() ? &target.dsvTable() : nullptr, 0U);

        target.switchToRenderAccessState(m_command_list);

        m_command_list.clearRenderTargetView(target.rtvTable(), 0, m_clear_color);

        m_command_list.close();

        m_rendering_tasks.m_device.defaultCommandQueue().executeCommandList(m_command_list);

        return true;
    }

    TaskType type() const override
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
    bool doTask(uint8_t worker_id, uint64_t current_frame_index) override
    {
        
        auto& target = *m_rendering_tasks.m_current_rendering_target_ptr;

        m_command_list.reset();

        target.switchToInitialState(m_command_list);

        m_command_list.close();
        m_rendering_tasks.m_device.defaultCommandQueue().executeCommandList(m_command_list);
        m_rendering_tasks.m_end_of_frame_gpu_wall.signalFromGPU(m_rendering_tasks.m_device.defaultCommandQueue());
        
        PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
            "CPU job for frame %i finish", current_frame_index);

        return true;
    }

    TaskType type() const override
    {
        return TaskType::gpu_draw;
    }

private:
    RenderingTasks& m_rendering_tasks;
    CommandList m_command_list;
};


RenderingTasks::RenderingTasks(Globals& globals)
    : m_dx_resources{ *globals.get<DxResourceFactory>() }
    , m_device{ *globals.get<Device>() }

    , m_task_graph{ globals.get<GlobalSettings>()->getNumberOfWorkers(), "RenderingTasksGraph" }
    , m_task_sink{ m_task_graph, convertFileStreamsToGenericStreams(globals.get<LoggingStreams>()->worker_logging_streams), "RenderingTasksSink" }

    , m_frame_begin_task{ new FrameBeginTask{*this, math::Vector4f{0.f, 0.f, 0.f, 0.f}} }
    , m_frame_end_task{ new FrameEndTask{*this} }

    , m_end_of_frame_cpu_wall{ m_device }
    , m_end_of_frame_gpu_wall{ m_device }
{
    m_task_graph.setRootNodes({ m_frame_begin_task.get() });
    m_frame_begin_task->addDependent(*m_frame_end_task);

    m_task_sink.start();
}

RenderingTasks::~RenderingTasks()
{
    m_task_sink.shutdown();
}


void RenderingTasks::render(RenderingTarget& target, 
    std::function<void(RenderingTarget const&)> const& presentation_routine)
{
    m_current_rendering_target_ptr = &target;
    m_task_sink.submit(m_end_of_frame_cpu_wall.lastValueSignaled());
    presentation_routine(target);

    m_end_of_frame_cpu_wall.signalFromCPU();
    m_end_of_frame_gpu_wall.signalFromGPU(m_device.defaultCommandQueue());
}

uint64_t RenderingTasks::dispatchedFramesCount() const
{
    return m_end_of_frame_cpu_wall.lastValueSignaled();
}

uint64_t RenderingTasks::completedFramesCount() const
{
    return m_end_of_frame_gpu_wall.lastValueSignaled();
}

void RenderingTasks::waitForFrameCompletion(uint64_t frame_idx) const
{
    m_end_of_frame_gpu_wall.waitUntilValue(frame_idx + 1);
}
