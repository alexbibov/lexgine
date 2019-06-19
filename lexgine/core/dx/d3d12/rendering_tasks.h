#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

#include <atomic>
#include <memory>
#include <functional>

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/viewport.h"
#include "lexgine/core/concurrency/task_sink.h"
#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/misc/static_vector.h"
#include "lexgine/core/dx/d3d12/tasks/rendering_tasks/lexgine_core_dx_d3d12_tasks_rendering_tasks_fwd.h"

#include "lexgine_core_dx_d3d12_fwd.h"
#include "signal.h"
#include "constant_buffer_stream.h"
#include "basic_rendering_services.h"

namespace lexgine::core::dx::d3d12 {

class RenderingTask : public concurrency::SchedulableTask
{
private:
    enum class tagRenderingConfigurationUpdateFlags : int
    {
        VIEWPORT_CHANGED = 0x1,
        COLOR_FORMAT_CHANGED = 0x2,
        DEPTH_FORMAT_CHANGED = 0x4,
        RENDERING_WINDOW_CHANGED = 0x8
    };

public:
    using renderingConfigurationUpdatedFlags = misc::Flags<tagRenderingConfigurationUpdateFlags>;

public:
    virtual void renderingConfigurationUpdated(renderingConfigurationUpdatedFlags update_flags);
};

class RenderingTaskFactory final
{
public:
    template<typename TaskType, typename ... Args>
    static std::shared_ptr<TaskType> create(Args ... args)
    {
        return TaskType::create(std::forward<Args>(args)...);
    }
};


class RenderingTasks : public NamedEntity<class_names::D3D12_RenderingTasks>
{
public:
    class GPURenderingServices;

public:
    RenderingTasks(Globals& globals);
    ~RenderingTasks();

    /*! Assigns default color and depth formats assumed for the rendering targets
     This may be used by some rendering tasks that need to assume certain color and
     depth formats for correct initialization of their associated pipeline state objects
    */
    void defineRenderingConfiguration(Viewport const& viewport,
        DXGI_FORMAT rendering_target_color_format, DXGI_FORMAT rendering_target_depth_format,
        osinteraction::windows::Window* p_rendering_window);

    void render(RenderingTarget& rendering_target, 
        std::function<void(RenderingTarget const&)> const& presentation_routine);

    BasicRenderingServices& basicRenderingServices() { return m_basic_rendering_services; }
    FrameProgressTracker const& frameProgressTracker() { return m_frame_progress_tracker; }

    void flush();    //! flushes the rendering pipeline making sure that all GPU job scheduled so far has been completed

private:
    void cleanup();    //! flushes the rendering pipeline and shutdowns the task submission sink

private:
    Globals& m_globals;
    Device& m_device;
    FrameProgressTracker& m_frame_progress_tracker;

    concurrency::TaskGraph m_task_graph;
    concurrency::TaskSink m_task_sink;

    BasicRenderingServices m_basic_rendering_services;
    osinteraction::windows::Window* m_rendering_window_ptr;

private:    // rendering tasks
    std::shared_ptr<tasks::rendering_tasks::UIDrawTask> m_ui_draw_task;
    std::shared_ptr<tasks::rendering_tasks::Profiler> m_profiler;
    std::unique_ptr<tasks::rendering_tasks::TestRenderingTask> m_test_rendering_task;
};

}

#endif // !LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

