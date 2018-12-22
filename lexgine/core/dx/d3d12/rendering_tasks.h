#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/concurrency/task_sink.h"

#include "lexgine_core_dx_d3d12_fwd.h"


namespace lexgine::core::dx::d3d12 {

class RenderingTasks final
{
public:
    RenderingTasks(Globals& globals);
    ~RenderingTasks();

    void run();

    void setRenderingTargets(RenderingTargetColor const* color_rendering_target,
        RenderingTargetDepth const* depth_rendering_target);


private:
    DxResourceFactory const& m_dx_resources;
    Device& m_device;
    RenderingTargetColor const* m_color_rendering_target_ptr;
    RenderingTargetDepth const* m_depth_rendering_target_ptr;


    concurrency::TaskGraph m_task_graph;
    concurrency::TaskSink m_task_sink;
};

}

#endif // !LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

