#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

#include <atomic>
#include <memory>
#include <functional>

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/concurrency/task_sink.h"

#include "lexgine_core_dx_d3d12_fwd.h"
#include "signal.h"

namespace lexgine::core::dx::d3d12 {

class RenderingTasks final : public NamedEntity<class_names::D3D12_RenderingTasks>
{
public:
    RenderingTasks(Globals& globals);
    ~RenderingTasks();

    void render(RenderingTarget& target, 
        std::function<void(RenderingTarget const&)> const& presentation_routine);

    uint64_t dispatchedFramesCount() const;
    uint64_t completedFramesCount() const;

    void waitForFrameCompletion(uint64_t frame_idx) const;

private:
    class FrameBeginTask;
    class FrameEndTask;

private:
    DxResourceFactory const& m_dx_resources;
    Device& m_device;

    concurrency::TaskGraph m_task_graph;
    concurrency::TaskSink m_task_sink;

    std::unique_ptr<FrameBeginTask> m_frame_begin_task;
    std::unique_ptr<FrameEndTask> m_frame_end_task;

    Signal m_end_of_frame_cpu_wall;
    Signal m_end_of_frame_gpu_wall;

    RenderingTarget* m_current_rendering_target_ptr;
};

}

#endif // !LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

