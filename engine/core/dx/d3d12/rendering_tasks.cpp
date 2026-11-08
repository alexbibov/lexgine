#include <format>

#include "rendering_tasks.h"

#include "engine/scenegraph/scene.h"

#include "engine/core/concurrency/thread_affinity.h"
#include "engine/core/globals.h"
#include "engine/core/global_settings.h"
#include "engine/core/profiling_services.h"
#include "engine/core/exception.h"
#include "engine/core/misc/misc.h"

#include "engine/core/dx/d3d12/rendering/gbuffer.h"
#include "engine/core/dx/d3d12/tasks/rendering_tasks/test_rendering_task.h"
#include "engine/core/dx/d3d12/tasks/rendering_tasks/ui_draw_task.h"
#include "engine/core/dx/d3d12/tasks/rendering_tasks/gpu_profiling_queries_flush_task.h"
#include "engine/core/dx/d3d12/tasks/rendering_tasks/gpu_work_execution_task.h"
#include "engine/core/ui/profiler.h"

#include "dx_resource_factory.h"
#include "descriptor_allocator.h"
#include "device.h"
#include "frame_progress_tracker.h"


namespace lexgine::core::dx::d3d12
{

namespace {

tasks::rendering_tasks::RenderingWork::RenderingConfigurationUpdateFlags getRenderingConfigurationUpdateFlags(
    RenderingConfiguration const& old_configuration,
    RenderingConfiguration const& new_configuration
)
{
    tasks::rendering_tasks::RenderingWork::RenderingConfigurationUpdateFlags flags{};
    if (old_configuration.viewport != new_configuration.viewport)
        flags |= tasks::rendering_tasks::RenderingWork::RenderingConfigurationUpdateFlags::base_values::viewport_changed;

    if (old_configuration.color_buffer_format != new_configuration.color_buffer_format)
        flags |= tasks::rendering_tasks::RenderingWork::RenderingConfigurationUpdateFlags::base_values::color_format_changed;

    if (old_configuration.depth_buffer_format != new_configuration.depth_buffer_format)
        flags |= tasks::rendering_tasks::RenderingWork::RenderingConfigurationUpdateFlags::base_values::depth_format_changed;

    return flags;
}

}

RenderingTasks::RenderingTasks(Globals& globals, osinteraction::windows::Window& rendering_window,
    std::function<bool(uint32_t)> adapter_selector)
    : m_globals{ globals }
    , m_device{ globals.device() }
    , m_frame_progress_tracker{ m_device.frameProgressTracker() }
    , m_task_graph{ globals.globalSettings().getNumberOfWorkers(), "RenderingTasksGraph" }
    , m_task_sink{ m_task_graph, "RenderingTasksSink",
                   [](uint8_t) { concurrency::registerSlaveWorkerThread(); } }
    , m_basic_rendering_services{ globals }
    , m_rendering_configuration{ Viewport{math::Vector2f{}, math::Vector2f{}, math::Vector2f{}},
                                 DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN }
    , m_adapter_selector{ std::move(adapter_selector) }
    , m_gbuffer{ std::make_unique<rendering::Gbuffer>(globals) }
{
    m_test_rendering_task_build_cmd_list = RenderingTaskFactory::create<tasks::rendering_tasks::TestRenderingTask>(m_globals, m_basic_rendering_services);
    m_ui_draw_build_cmd_list = RenderingTaskFactory::create<tasks::rendering_tasks::UIDrawTask>(globals, m_basic_rendering_services);
    m_ui_draw_build_cmd_list->bindRenderingWindow(rendering_window);
    m_console = ui::Console::create(globals, m_basic_rendering_services, m_task_graph);
    rendering_window.addListener(m_console);
    registerConsoleCommands();
    m_gpu_profiling_queries_flush_build_cmd_list = RenderingTaskFactory::create<tasks::rendering_tasks::GpuProfilingQueriesFlushTask>(globals);
    m_profiler = RenderingTaskFactory::create<ui::Profiler>(globals, m_basic_rendering_services, m_task_graph);
    m_ui_draw_build_cmd_list->addUIProvider(m_profiler);
    m_ui_draw_build_cmd_list->addUIProvider(m_console);

    m_post_rendering_gpu_tasks = RenderingTaskFactory::create<tasks::rendering_tasks::GpuWorkExecutionTask>(m_device,
        "GPU draw tasks", m_basic_rendering_services);
    m_post_rendering_gpu_tasks->addSource(*m_test_rendering_task_build_cmd_list);
    m_post_rendering_gpu_tasks->addSource(*m_ui_draw_build_cmd_list);

    m_gpu_profiling_queries_flush_task = RenderingTaskFactory::create<tasks::rendering_tasks::GpuWorkExecutionTask>(m_device,
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

    if (flags.isSet(tasks::rendering_tasks::RenderingWork::RenderingConfigurationUpdateFlags::base_values::color_format_changed)
        || flags.isSet(tasks::rendering_tasks::RenderingWork::RenderingConfigurationUpdateFlags::base_values::depth_format_changed))
    {
        BasicRenderingServicesAttorney<RenderingTasks>::defineRenderingTargetFormat(m_basic_rendering_services,
            rendering_configuration.color_buffer_format, rendering_configuration.depth_buffer_format);
    }

    if (flags.isSet(tasks::rendering_tasks::RenderingWork::RenderingConfigurationUpdateFlags::base_values::viewport_changed))
    {
        BasicRenderingServicesAttorney<RenderingTasks>::defineRenderingViewport(m_basic_rendering_services, rendering_configuration.viewport);
    }


    if (flags.isSet(tasks::rendering_tasks::RenderingWork::RenderingConfigurationUpdateFlags::base_values::viewport_changed))
    {
        auto gbuffer_width = static_cast<uint32_t>(rendering_configuration.viewport.width());
        auto gbuffer_height = static_cast<uint32_t>(rendering_configuration.viewport.height());
        if (gbuffer_width && gbuffer_height) m_gbuffer->init(gbuffer_width, gbuffer_height);
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
    concurrency::assertOnMasterWorkerThread();

    // Submit and present frame

    BasicRenderingServicesAttorney<RenderingTasks>::defineFrameContext(m_basic_rendering_services,
        m_frame_progress_tracker.currentFrameIndex(), rendering_target);

    // the scene may only change between the frames, never while the workers are recording one
    m_render_command_pipe.drain();

    m_device.transientDescriptorAllocator().nextFrame();
    m_device.queryCache()->markFrameBegin();
    m_task_sink.submit(m_frame_progress_tracker.currentFrameIndex());
    presenter();
    m_device.queryCache()->markFrameEnd();
}

void RenderingTasks::flush()
{
    // no thread role is asserted here: this is reached from the rendering thread before it leaves its loop
    // and from cleanup(), which the owning thread runs while the engine is torn down

    m_frame_progress_tracker.synchronize();
}

void RenderingTasks::cleanup()
{
    // no thread role is asserted here: this is reached both from a rendering configuration change, which
    // happens on the rendering thread, and from the destructor, which runs on the owning thread

    if (m_task_sink.isRunning()) m_task_sink.shutdown();
    flush();
}


std::optional<RenderingTasks::SceneSource> RenderingTasks::currentSceneSource() const
{
    std::lock_guard lock{ m_simulation_thread_mutex };
    return m_current_scene_source;
}

bool RenderingTasks::requestSceneLoad(SceneSource const& scene_source)
{
    return m_render_command_pipe.enqueue([this, scene_source]() { loadScene(scene_source); });
}

void RenderingTasks::loadScene(SceneSource const& scene_source)
{
    concurrency::assertOnMasterWorkerThread();

    std::string const scene_description = std::visit([&scene_source](auto const& scene) -> std::string
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(scene)>, std::string>)
            {
                return scene_source.path + " (\"" + scene + "\")";
            }
            else
            {
                return scene_source.path;
            }
        }, scene_source.scene);

    m_current_scene = std::visit([this, &scene_source](auto const& scene)
        {
            return scenegraph::Scene::loadScene(m_globals, m_basic_rendering_services, scene_source.path, scene);
        }, scene_source.scene);

    {
        std::lock_guard lock{ m_simulation_thread_mutex };
        m_current_scene_source = m_current_scene ? std::optional<SceneSource>{ scene_source } : std::nullopt;
    }

    if (m_current_scene)
    {
        misc::Log::retrieve()->out("Scene loaded: " + scene_description, misc::LogMessageType::information);
        return;
    }

    misc::Log::retrieve()->out("ERROR: failed to load scene: " + scene_description, misc::LogMessageType::error);
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

            if (!requestSceneLoad(SceneSource{ .path = path, .scene = id }))
            {
                return { .succeeded = false, .msg = "Unable to schedule loading of the scene: " + path };
            }

            return { .succeeded = true };
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

            if (!requestSceneLoad(SceneSource{ .path = path, .scene = name }))
            {
                return { .succeeded = false, .msg = "Unable to schedule loading of the scene: " + path };
            }

            return { .succeeded = true };
        });

    reg.addNamespace("device", "Device management commands");

    // device.list
    interaction::console::CommandSpec list_cmd{ .name = "list",
        .summary = "List the adapters the device can be created on, the current one marked with '*'" };
    reg.addCommand("device", list_cmd,
        [this](interaction::console::ArgMap const&) -> interaction::console::CommandExecResult
        {
            auto const& adapter_enumerator = m_globals.dxResourceFactory().hardwareAdapterEnumerator();

            std::string adapter_list{ "Adapters:" };
            for (uint32_t adapter_id = 0; adapter_id < adapter_enumerator.getAdapterCount(); ++adapter_id)
            {
                auto const& adapter = adapter_enumerator[adapter_id];
                auto const details = adapter->getProperties().details;
                adapter_list += std::format("\n{} [{}] {} (LUID {:08X}:{:08X})",
                    adapter.get() == m_device.hwAdapter() ? '*' : ' ', adapter_id,
                    misc::wstringToAsciiString(details.name),
                    static_cast<uint32_t>(details.luid.HighPart), static_cast<uint32_t>(details.luid.LowPart));
            }

            misc::Log::retrieve()->out(adapter_list, misc::LogMessageType::information);
            return { .succeeded = true };
        });

    // device.switch adapter_id=<int>
    interaction::console::CommandSpec switch_cmd{ .name = "switch",
        .summary = "Move the rendering onto another adapter" };
    switch_cmd.args.push_back({ .name = "adapter_id",
        .description = "Identifier of the adapter as printed by device.list",
        .parser = interaction::console::value_parsers::int_parser });
    reg.addCommand("device", switch_cmd,
        [this](interaction::console::ArgMap const& args) -> interaction::console::CommandExecResult
        {
            int const adapter_id = std::get<int>(args.at("adapter_id"));
            if (adapter_id < 0 || !m_adapter_selector(static_cast<uint32_t>(adapter_id)))
            {
                return { .succeeded = false, .msg = std::format("There is no adapter with identifier {}", adapter_id) };
            }

            return { .succeeded = true };
        });

#ifdef _DEBUG
    // device.remove
    interaction::console::CommandSpec remove_cmd{ .name = "remove",
        .summary = "Remove the device between the frames to exercise the device loss recovery" };
    reg.addCommand("device", remove_cmd,
        [this](interaction::console::ArgMap const&) -> interaction::console::CommandExecResult
        {
            bool const scheduled = m_render_command_pipe.enqueue([this]()
                {
                    Microsoft::WRL::ComPtr<ID3D12Device5> device5;
                    if (SUCCEEDED(m_device.native().As(&device5))) device5->RemoveDevice();
                });

            if (!scheduled) return { .succeeded = false, .msg = "Unable to schedule removal of the device" };
            return { .succeeded = true };
        });
#endif
}



}