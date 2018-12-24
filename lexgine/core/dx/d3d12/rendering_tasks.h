#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/concurrency/task_sink.h"

#include "lexgine_core_dx_d3d12_fwd.h"


namespace lexgine::core::dx::d3d12 {

class RenderingTasks final : public NamedEntity<class_names::D3D12_RenderingTasks>
{
public:
    RenderingTasks(Globals& globals);
    ~RenderingTasks();

    void run();

    void setRenderingTargets(RenderingTargetColor const* color_rendering_target,
        RenderingTargetDepth const* depth_rendering_target);

private:
    class FrameBeginTask;
    class FrameEndTask;

private:
    DxResourceFactory const& m_dx_resources;
    Device& m_device;
    RenderingTargetColor const* m_color_rendering_target_ptr;
    RenderingTargetDepth const* m_depth_rendering_target_ptr;

    concurrency::TaskGraph m_task_graph;
    concurrency::TaskSink m_task_sink;

    std::unique_ptr<FrameBeginTask> m_frame_begin_task;
    std::unique_ptr<FrameEndTask> m_frame_end_task;
};

}

#endif // !LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

