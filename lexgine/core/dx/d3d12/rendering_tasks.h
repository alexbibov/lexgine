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
#include "constant_buffer_stream.h"

namespace lexgine::core::dx::d3d12 {

class RenderingTasks final : public NamedEntity<class_names::D3D12_RenderingTasks>
{
public:
    RenderingTasks(Globals& globals);
    ~RenderingTasks();

    /*! Assigns default color and depth formats assumed for the rendering targets
     This may be used by some rendering tasks that need to assume certain color and
     depth formats for correct initialization of their associated pipeline state objects
    */
    void setDefaultColorAndDepthFormats(DXGI_FORMAT default_color_format, DXGI_FORMAT default_depth_format);

    void render(RenderingTarget& target, 
        std::function<void(RenderingTarget const&)> const& presentation_routine);

    FrameProgressTracker const& frameProgressTracker() const;

private:
    class FrameBeginTask;
    class FrameEndTask;

    class TestRendering;    // To be removed, here for testing purposes only

private:
    Globals& m_globals;
    DxResourceFactory& m_dx_resources;
    Device& m_device;
    FrameProgressTracker& m_frame_progress_tracker;

    concurrency::TaskGraph m_task_graph;
    concurrency::TaskSink m_task_sink;

    ConstantBufferStream m_constant_data_stream;

    std::unique_ptr<FrameBeginTask> m_frame_begin_task;
    std::unique_ptr<FrameEndTask> m_frame_end_task;
    std::unique_ptr<TestRendering> m_test_triangle_rendering;

    RenderingTarget* m_current_rendering_target_ptr;
    DXGI_FORMAT m_default_color_format;
    DXGI_FORMAT m_default_depth_format;
};

}

#endif // !LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

