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

RenderingWork::RenderingConfigurationUpdateFlags getRenderingConfigurationUpdateFlags(RenderingConfiguration const& old_configuration,
    RenderingConfiguration const& new_configuration)
{
    RenderingWork::RenderingConfigurationUpdateFlags flags{};
    if (old_configuration.viewport != new_configuration.viewport)
        flags |= RenderingWork::RenderingConfigurationUpdateFlags::base_values::viewport_changed;

    if (old_configuration.color_buffer_format != new_configuration.color_buffer_format)
        flags |= RenderingWork::RenderingConfigurationUpdateFlags::base_values::color_format_changed;

    if (old_configuration.depth_buffer_format != new_configuration.depth_buffer_format)
        flags |= RenderingWork::RenderingConfigurationUpdateFlags::base_values::depth_format_changed;

    if (old_configuration.p_rendering_window != new_configuration.p_rendering_window)
        flags |= RenderingWork::RenderingConfigurationUpdateFlags::base_values::rendering_window_changed;

    return flags;
}

}

RenderingTasks::RenderingTasks(Globals& globals)
    : m_globals{ globals }
    , m_device{ *globals.get<Device>() }
    , m_frame_progress_tracker{ globals.get<DxResourceFactory>()->retrieveFrameProgressTracker(m_device) }
    , m_task_graph{ globals.get<GlobalSettings>()->getNumberOfWorkers(), "RenderingTasksGraph" }
    , m_task_sink{ m_task_graph, convertFileStreamsToGenericStreams(globals.get<LoggingStreams>()->worker_logging_streams), "RenderingTasksSink" }
    , m_basic_rendering_services{ globals }
    , m_rendering_configuration{ Viewport{math::Vector2f{}, math::Vector2f{}, math::Vector2f{}}, 
                                 DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, nullptr}
{
    m_test_rendering_task = RenderingTaskFactory::create<TestRenderingTask>(m_globals, m_basic_rendering_services);
    m_ui_draw_task = RenderingTaskFactory::create<UIDrawTask>(globals, m_basic_rendering_services);
    m_profiler = RenderingTaskFactory::create<Profiler>(globals, m_task_graph);

    m_post_rendering_gpu_tasks = RenderingTaskFactory::create<GpuWorkExecutionTask>(m_device, "GPU draw tasks", m_frame_progress_tracker, m_basic_rendering_services);
    m_post_rendering_gpu_tasks->addSource(*m_test_rendering_task);
    m_post_rendering_gpu_tasks->addSource(*m_ui_draw_task);

    m_ui_draw_task->addUIProvider(m_profiler);

    m_task_graph.setRootNodes({ 
        ROOT_NODE_CAST(m_test_rendering_task.get()), 
        ROOT_NODE_CAST(m_ui_draw_task.get()) 
        });
    m_test_rendering_task->addDependent(*m_post_rendering_gpu_tasks);
    m_ui_draw_task->addDependent(*m_post_rendering_gpu_tasks);

    // m_task_sink.start();
}

RenderingTasks::~RenderingTasks()
{
    cleanup();
}

void RenderingTasks::defineRenderingConfiguration(RenderingConfiguration const& rendering_configuration)
{
    auto flags = getRenderingConfigurationUpdateFlags(m_rendering_configuration, rendering_configuration);

    if (flags.isSet(RenderingWork::RenderingConfigurationUpdateFlags::base_values::color_format_changed)
        || flags.isSet(RenderingWork::RenderingConfigurationUpdateFlags::base_values::depth_format_changed))
    {
        BasicRenderingServicesAttorney<RenderingTasks>::defineRenderingTargetFormat(m_basic_rendering_services, 
            rendering_configuration.color_buffer_format, rendering_configuration.depth_buffer_format);
    }
    
    if (flags.isSet(RenderingWork::RenderingConfigurationUpdateFlags::base_values::viewport_changed))
    {
        BasicRenderingServicesAttorney<RenderingTasks>::defineRenderingViewport(m_basic_rendering_services, rendering_configuration.viewport);
    }
    

    if (flags.isSet(RenderingWork::RenderingConfigurationUpdateFlags::base_values::rendering_window_changed))
    {
        BasicRenderingServicesAttorney<RenderingTasks>::defineRenderingWindow(m_basic_rendering_services, rendering_configuration.p_rendering_window);
    }
    

    if(flags.getValue())
    {
        cleanup();

        m_rendering_configuration = rendering_configuration;

        // update rendering tasks
        m_test_rendering_task->updateRenderingConfiguration(flags, m_rendering_configuration);
        m_ui_draw_task->updateRenderingConfiguration(flags, m_rendering_configuration);
        
        m_task_sink.start();
    }
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



