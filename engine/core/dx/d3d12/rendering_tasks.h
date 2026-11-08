#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

#include <atomic>
#include <memory>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <variant>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/ui/lexgine_core_ui_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/rendering_configuration.h"
#include "engine/core/rendering/render_command_pipe.h"
#include "engine/core/dx/dxgi/lexgine_core_dx_dxgi_fwd.h"
#include "engine/core/concurrency/task_sink.h"
#include "engine/core/concurrency/schedulable_task.h"
#include "engine/core/misc/static_vector.h"
#include "engine/core/dx/d3d12/tasks/rendering_tasks/lexgine_core_dx_d3d12_tasks_rendering_tasks_fwd.h"
#include "engine/core/dx/d3d12/tasks/rendering_tasks/rendering_work.h"
#include "engine/core/ui/console.h"
#include "engine/core/dx/d3d12/rendering/lexgine_core_dx_d3d12_rendering_fwd.h"
#include "engine/scenegraph/lexgine_scenegraph_fwd.h"

#include "lexgine_core_dx_d3d12_fwd.h"
#include "basic_rendering_services.h"


namespace lexgine::core::dx::d3d12 {

class RenderingTaskFactory final
{
public:
    template<typename TaskType, typename ... Args>
    static std::shared_ptr<TaskType> create(Args&& ... args)
    {
        return TaskType::create(std::forward<Args>(args)...);
    }
};


class RenderingTasks : public NamedEntity<RenderingTasks>
{
public:
    class GPURenderingServices;

    //! Identifies a scene within a GLTF/GLB file, either by its index or by its name
    struct SceneSource
    {
        std::string path;
        std::variant<unsigned, std::string> scene;
    };

public:
    /*! @param adapter_selector requests the rendering to move onto the adapter with the given identifier and
     returns 'false' if there is no such adapter; the console reaches it through the device commands
    */
    RenderingTasks(Globals& globals, osinteraction::windows::Window& rendering_window,
        std::function<bool(uint32_t)> adapter_selector);
    ~RenderingTasks();

    /*! Assigns default color and depth formats assumed for the rendering targets
     This may be used by some rendering tasks that need to assume certain color and
     depth formats for correct initialization of their associated pipeline state objects
    */
    void defineRenderingConfiguration(RenderingConfiguration const& rendering_configuration);

    /*! records the frame into @param rendering_target and submits it. The frame is presented by @param presenter,
     which reports the progress of the presentation
    */
    void render(RenderingTarget& rendering_target, std::function<dxgi::PresentStatistics()> const& presenter);

    BasicRenderingServices& basicRenderingServices() { return m_basic_rendering_services; }

    FrameProgressTracker& frameProgressTracker() { return m_frame_progress_tracker; }
    FrameProgressTracker const& frameProgressTracker() const { return m_frame_progress_tracker; }

    void flush();    //! flushes the rendering pipeline making sure that all GPU job scheduled so far has been completed

    /*! returns the source of the current scene, if a scene is loaded. The scene may be replaced at any time,
     so the returned source may already be obsolete when the caller uses it
    */
    std::optional<SceneSource> currentSceneSource() const;

    /*! schedules the scene identified by @param scene_source to replace the current scene between the frames.
     Returns 'false' if the request could not be scheduled
    */
    bool requestSceneLoad(SceneSource const& scene_source);

private:
    void cleanup();    //! flushes the rendering pipeline and shutdowns the task submission sink
    void registerConsoleCommands();

    void loadScene(SceneSource const& scene_source);    //! replaces the current scene, between the frames

private:
    Globals& m_globals;
    Device& m_device;
    FrameProgressTracker& m_frame_progress_tracker;

    concurrency::TaskGraph m_task_graph;
    concurrency::TaskSink m_task_sink;

    BasicRenderingServices m_basic_rendering_services;
    core::rendering::RenderCommandPipe m_render_command_pipe;    //!< carries the state changes requested while a frame is recorded
    RenderingConfiguration m_rendering_configuration;
    std::function<bool(uint32_t)> m_adapter_selector;

private:    // rendering tasks
    std::shared_ptr<tasks::rendering_tasks::UIDrawTask> m_ui_draw_build_cmd_list;
    std::shared_ptr<ui::Console> m_console;
    std::shared_ptr<ui::Profiler> m_profiler;
    std::shared_ptr<tasks::rendering_tasks::TestRenderingTask> m_test_rendering_task_build_cmd_list;
    std::shared_ptr<tasks::rendering_tasks::GpuProfilingQueriesFlushTask> m_gpu_profiling_queries_flush_build_cmd_list;

    std::shared_ptr<tasks::rendering_tasks::GpuWorkExecutionTask> m_post_rendering_gpu_tasks;
    std::shared_ptr<tasks::rendering_tasks::GpuWorkExecutionTask> m_gpu_profiling_queries_flush_task;

    std::shared_ptr<scenegraph::Scene> m_current_scene;

    /*!< guards the state shared with the simulation thread. Every access to that state takes the lock, whichever
     thread makes it
    */
    mutable std::mutex m_simulation_thread_mutex;
    std::optional<SceneSource> m_current_scene_source;    //!< guarded by m_simulation_thread_mutex
    std::unique_ptr<rendering::Gbuffer> m_gbuffer;
};

}

#endif // !LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

