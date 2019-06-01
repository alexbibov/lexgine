#include "rendering_tasks.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/logging_streams.h"
#include "lexgine/core/exception.h"

#include "lexgine/core/dx/d3d12/tasks/rendering_tasks/test_rendering_task.h"
#include "lexgine/core/dx/d3d12/tasks/rendering_tasks/ui_draw_task.h"
#include "lexgine/core/dx/d3d12/tasks/rendering_tasks/profiler.h"

#include "dx_resource_factory.h"
#include "device.h"
#include "frame_progress_tracker.h"


using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::math;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;


namespace {

std::vector<std::ostream*> convertFileStreamsToGenericStreams(std::vector<std::ofstream>& fstreams)
{
    std::vector<std::ostream*> res(fstreams.size());
    std::transform(fstreams.begin(), fstreams.end(), res.begin(), [](std::ofstream & fs)->std::ostream * { return &fs; });
    return res;
}

}

RenderingTasks::RenderingTasks(Globals& globals)
    : m_globals{ globals }
    , m_device{ *globals.get<Device>() }
    , m_frame_progress_tracker{ globals.get<DxResourceFactory>()->retrieveFrameProgressTracker(m_device) }
    , m_task_graph{ globals.get<GlobalSettings>()->getNumberOfWorkers(), "RenderingTasksGraph" }
    , m_task_sink{ m_task_graph, convertFileStreamsToGenericStreams(globals.get<LoggingStreams>()->worker_logging_streams), "RenderingTasksSink" }
    , m_basic_rendering_services{ globals }
{
    m_test_rendering_task.reset(new TestRenderingTask{ m_globals, m_basic_rendering_services });
}

RenderingTasks::~RenderingTasks()
{
    cleanup();
}

void RenderingTasks::defineRenderingConfiguration(Viewport const& viewport,
    DXGI_FORMAT rendering_target_color_format, DXGI_FORMAT rendering_target_depth_format,
    osinteraction::windows::Window* p_rendering_window/* = nullptr*/)
{
    cleanup();

    BasicRenderingServicesAttorney<RenderingTasks>
        ::defineRenderingTargetFormat(m_basic_rendering_services,
            rendering_target_color_format, rendering_target_depth_format);

    BasicRenderingServicesAttorney<RenderingTasks>
        ::defineRenderingViewport(m_basic_rendering_services, viewport);

    {
        // update rendering tasks
        m_test_rendering_task->updateBufferFormats(rendering_target_color_format, rendering_target_depth_format);

        if (p_rendering_window == nullptr)
        {
            m_ui_draw_task.reset();

        }
        else
        {
            m_ui_draw_task = UIDrawTask::create(m_globals, m_basic_rendering_services, *p_rendering_window);
            m_profiler = tasks::rendering_tasks::Profiler::create();
            m_ui_draw_task->addUIProvider(m_profiler);
        }

        m_ui_draw_task->updateBufferFormats(rendering_target_color_format, rendering_target_depth_format);
    }
    
    m_task_graph.setRootNodes({ m_test_rendering_task.get() });

    if (p_rendering_window)
    {
        m_test_rendering_task->addDependent(*m_ui_draw_task);
        m_ui_draw_task->addDependent(*m_profiler);
    }

    m_task_sink.start();
}

void RenderingTasks::render(RenderingTarget& rendering_target,
    std::function<void(RenderingTarget const&)> const& presentation_routine)
{
    {
        // Begin frame

        FrameProgressTrackerAttorney<RenderingTasks>::signalCPUBeginFrame(m_frame_progress_tracker);
        FrameProgressTrackerAttorney<RenderingTasks>::signalGPUBeginFrame(m_frame_progress_tracker,
            m_device.defaultCommandQueue());
    }
    
    {
        // Submit and present frame

        BasicRenderingServicesAttorney<RenderingTasks>
            ::defineRenderingTarget(m_basic_rendering_services, rendering_target);

        m_task_sink.submit(m_frame_progress_tracker.currentFrameIndex());
        presentation_routine(rendering_target);
    }

    {
        // End frame

        FrameProgressTrackerAttorney<RenderingTasks>::signalGPUEndFrame(m_frame_progress_tracker, m_device.defaultCommandQueue());
        FrameProgressTrackerAttorney<RenderingTasks>::signalCPUEndFrame(m_frame_progress_tracker);
    }
}

void RenderingTasks::flush()
{
    m_frame_progress_tracker.waitForFrameCompletion(m_frame_progress_tracker.lastScheduledFrameIndex());
}

void RenderingTasks::cleanup()
{
    if (m_task_sink.isRunning()) m_task_sink.shutdown();
    flush();
}



