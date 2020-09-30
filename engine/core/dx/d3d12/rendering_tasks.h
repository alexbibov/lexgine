#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

#include <atomic>
#include <memory>
#include <functional>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/ui/lexgine_core_ui_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/class_names.h"
#include "engine/core/rendering_configuration.h"
#include "engine/core/concurrency/task_sink.h"
#include "engine/core/concurrency/schedulable_task.h"
#include "engine/core/misc/static_vector.h"
#include "engine/core/dx/d3d12/tasks/rendering_tasks/lexgine_core_dx_d3d12_tasks_rendering_tasks_fwd.h"
#include "engine/core/dx/d3d12/tasks/rendering_tasks/rendering_work.h"

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
    void defineRenderingConfiguration(RenderingConfiguration const& rendering_configuration);

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
    RenderingConfiguration m_rendering_configuration;

private:    // rendering tasks
    std::shared_ptr<tasks::rendering_tasks::UIDrawTask> m_ui_draw_build_cmd_list;
    std::shared_ptr<ui::Profiler> m_profiler;
    std::shared_ptr<tasks::rendering_tasks::TestRenderingTask> m_test_rendering_task_build_cmd_list;
    std::shared_ptr<tasks::rendering_tasks::GpuProfilingQueriesFlushTask> m_gpu_profiling_queries_flush_build_cmd_list;

    std::shared_ptr<tasks::rendering_tasks::GpuWorkExecutionTask> m_post_rendering_gpu_tasks;
    std::shared_ptr<tasks::rendering_tasks::GpuWorkExecutionTask> m_gpu_profiling_queries_flush_task;
};

}

#endif // !LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

