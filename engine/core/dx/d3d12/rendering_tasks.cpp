#include "engine/scenegraph/scene.h"

#include "rendering_tasks.h"
#include "engine/core/globals.h"
#include "engine/core/global_settings.h"
#include "engine/core/profiling_services.h"
#include "engine/core/exception.h"

#include "engine/core/dx/d3d12/tasks/rendering_tasks/test_rendering_task.h"
#include "engine/core/dx/d3d12/tasks/rendering_tasks/ui_draw_task.h"
#include "engine/core/dx/d3d12/tasks/rendering_tasks/gpu_profiling_queries_flush_task.h"
#include "engine/core/dx/d3d12/tasks/rendering_tasks/gpu_work_execution_task.h"
#include "engine/core/ui/profiler.h"

#include "dx_resource_factory.h"
#include "device.h"
#include "frame_progress_tracker.h"


using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::ui;
using namespace lexgine::core::math;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;


namespace {

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
    , m_frame_progress_tracker{ m_device.frameProgressTracker() }
    , m_task_graph{ globals.get<GlobalSettings>()->getNumberOfWorkers(), "RenderingTasksGraph" }
    , m_task_sink{ m_task_graph, "RenderingTasksSink" }
    , m_basic_rendering_services{ globals }
    , m_rendering_configuration{ Viewport{math::Vector2f{}, math::Vector2f{}, math::Vector2f{}},
                                 DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, nullptr }
{
    m_test_rendering_task_build_cmd_list = RenderingTaskFactory::create<TestRenderingTask>(m_globals, m_basic_rendering_services);
    m_ui_draw_build_cmd_list = RenderingTaskFactory::create<UIDrawTask>(globals, m_basic_rendering_services);
    m_console = ui::Console::create(globals, m_basic_rendering_services, m_task_graph);
    registerConsoleCommands();
    m_gpu_profiling_queries_flush_build_cmd_list = RenderingTaskFactory::create<GpuProfilingQueriesFlushTask>(globals);
    m_profiler = RenderingTaskFactory::create<Profiler>(globals, m_basic_rendering_services, m_task_graph);
    m_ui_draw_build_cmd_list->addUIProvider(m_profiler);
    m_ui_draw_build_cmd_list->addUIProvider(m_console);

    m_post_rendering_gpu_tasks = RenderingTaskFactory::create<GpuWorkExecutionTask>(m_device,
        "GPU draw tasks", m_basic_rendering_services);
    m_post_rendering_gpu_tasks->addSource(*m_test_rendering_task_build_cmd_list);
    m_post_rendering_gpu_tasks->addSource(*m_ui_draw_build_cmd_list);

    m_gpu_profiling_queries_flush_task = RenderingTaskFactory::create<GpuWorkExecutionTask>(m_device,
        "Flush profiling events", m_basic_rendering_services, false);

    m_gpu_profiling_queries_flush_task->addSource(*m_gpu_profiling_queries_flush_build_cmd_list);

    m_task_graph.setRootNodes({
        ROOT_NODE_CAST(m_test_rendering_task_build_cmd_list.get()),
        ROOT_NODE_CAST(m_ui_draw_build_cmd_list.get())
        });
    m_test_rendering_task_build_cmd_list->addDependent(*m_post_rendering_gpu_tasks);
    m_ui_draw_build_cmd_list->addDependent(*m_post_rendering_gpu_tasks);
    m_ui_draw_build_cmd_list->addDependent(*m_gpu_profiling_queries_flush_build_cmd_list);
    m_gpu_profiling_queries_flush_build_cmd_list->addDependent(*m_gpu_profiling_queries_flush_task);
    m_post_rendering_gpu_tasks->addDependent(*m_gpu_profiling_queries_flush_task);

    ProfilingService::initializeProfilingServices(globals);
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
        rendering_configuration.p_rendering_window->addListener(m_console);
    }


    if (flags.getValue())
    {
        cleanup();

        m_rendering_configuration = rendering_configuration;

        // update rendering tasks
        m_test_rendering_task_build_cmd_list->updateRenderingConfiguration(flags, m_rendering_configuration);
        m_ui_draw_build_cmd_list->updateRenderingConfiguration(flags, m_rendering_configuration);

        m_task_sink.start();
    }
}

void RenderingTasks::render(RenderingTarget& rendering_target, const std::function<void(void)>& presenter)
{
    // Submit and present frame

    BasicRenderingServicesAttorney<RenderingTasks>::defineRenderingTarget(m_basic_rendering_services, rendering_target);

    m_device.queryCache()->markFrameBegin();
    m_task_sink.submit(m_frame_progress_tracker.currentFrameIndex());
    presenter();
    m_device.queryCache()->markFrameEnd();
}

void RenderingTasks::flush()
{
    if (m_frame_progress_tracker.started())
    {
        m_frame_progress_tracker.waitForFrameCompletion(m_frame_progress_tracker.lastScheduledFrameIndex());
    }
}

void RenderingTasks::cleanup()
{
    if (m_task_sink.isRunning()) m_task_sink.shutdown();
    flush();
}


void RenderingTasks::registerConsoleCommands()
{
    auto& reg = m_console->consoleCommandRegistry();
    reg.addNamespace("scene", "Scene management commands");

    // scene.load path=<path> [scene_id=<int>]
    interaction::console::CommandSpec load_cmd{ .name = "load",
        .summary = "Load a GLTF/GLB scene by index" };
    load_cmd.args.push_back({ .name = "path",
        .description = "Path to the .gltf or .glb file",
        .parser = interaction::console::value_parsers::string_parser });
    load_cmd.args.push_back({ .name = "scene_id",
        .description = "Index of the scene within the file (default: 0)",
        .parser = interaction::console::value_parsers::int_parser,
        ._default = interaction::console::ArgValue{ 0 } });
    reg.addCommand("scene", load_cmd,
        [this](interaction::console::ArgMap const& args) -> interaction::console::CommandExecResult
        {
            std::string path = std::get<std::string>(args.at("path"));
            unsigned id = static_cast<unsigned>(std::get<int>(args.at("scene_id")));
            m_current_scene = scenegraph::Scene::loadScene(m_globals, m_basic_rendering_services, path, id);
            if (m_current_scene)
            {
                misc::Log::retrieve()->out("Scene loaded: " + path, misc::LogMessageType::information);
                return { .succeeded = true };
            }
            return { .succeeded = false, .msg = "Failed to load scene: " + path };
        });

    // scene.load_named path=<path> name=<string>
    interaction::console::CommandSpec load_named_cmd{ .name = "load_named",
        .summary = "Load a GLTF/GLB scene by name" };
    load_named_cmd.args.push_back({ .name = "path",
        .description = "Path to the .gltf or .glb file",
        .parser = interaction::console::value_parsers::string_parser });
    load_named_cmd.args.push_back({ .name = "name",
        .description = "Name of the scene within the file",
        .parser = interaction::console::value_parsers::string_parser });
    reg.addCommand("scene", load_named_cmd,
        [this](interaction::console::ArgMap const& args) -> interaction::console::CommandExecResult
        {
            std::string path = std::get<std::string>(args.at("path"));
            std::string name = std::get<std::string>(args.at("name"));
            m_current_scene = scenegraph::Scene::loadScene(m_globals, m_basic_rendering_services, path, name);
            if (m_current_scene)
            {
                misc::Log::retrieve()->out("Scene loaded: " + path + " (\"" + name + "\")",
                    misc::LogMessageType::information);
                return { .succeeded = true };
            }
            return { .succeeded = false, .msg = "Failed to load scene \"" + name + "\": " + path };
        });
}



