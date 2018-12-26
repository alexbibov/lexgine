#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

#include <atomic>
#include <memory>

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

    void run();

    void dispatchExitSignal();

    void setRenderingTargets(std::vector<std::shared_ptr<RenderingTarget>> const& multiframe_targets);

    /*! signals the rendering tasks producing thread that a frame has been consumed.
     After the frame is consumed it cannot be accessed for reading any longer and its
     contents should be considered discarded
    */
    void consumeFrame();

    void waitUntilFrameIsReady(uint64_t frame_index) const;    //! blocks calling thread until the frame with the specified index is completed

    uint64_t totalFramesScheduled() const;
    uint64_t totalFramesRendered() const;
    uint64_t totalFramesConsumed() const;

private:
    class FrameBeginTask;
    class FrameEndTask;

private:
    DxResourceFactory const& m_dx_resources;
    Device& m_device;
    std::vector<std::shared_ptr<RenderingTarget>> m_targets;

    concurrency::TaskGraph m_task_graph;
    concurrency::TaskSink m_task_sink;

    std::unique_ptr<FrameBeginTask> m_frame_begin_task;
    std::unique_ptr<FrameEndTask> m_frame_end_task;

    uint16_t m_queued_frames_count;
    Signal m_end_of_frame_cpu_wall;
    Signal m_end_of_frame_gpu_wall;
    Signal m_frame_consumed_wall;
};

}

#endif // !LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

