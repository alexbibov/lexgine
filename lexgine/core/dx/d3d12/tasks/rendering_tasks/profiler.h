#ifndef LEXGINE_CORE_DX_D3D12_TASK_RENDERING_TASKS_PROFILER_H
#define LEXGINE_CORE_DX_D3D12_TASK_RENDERING_TASKS_PROFILER_H

#include <d3d12.h>
#include <pix3.h>

#include "3rd_party/imgui/imgui.h"
#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/ui.h"


namespace lexgine::core::dx::d3d12 {

namespace pix_marker_colors {

extern uint32_t const PixCPUJobMarkerColor;    // dark blue
extern uint32_t const PixGPUGeneralJobColor;    // purple
extern uint32_t const PixGPUGraphicsJobMarkerColor;    // yellow
extern uint32_t const PixGPUComputeJobMarkerColor;    // dark red
extern uint32_t const PixGPUCopyJobMarkerColor;    // gray

}

}


namespace lexgine::core::dx::d3d12::tasks::rendering_tasks {

class Profiler : public concurrency::SchedulableTask, public UIProvider
{
    using lexgine::core::Entity::getId;

public:
    static std::shared_ptr<Profiler> create()
    {
        return std::shared_ptr<Profiler>{new Profiler{}};
    }

public:    // required by UIProvider
    void constructUI() const override;

private:    // required by AbstractTask interface
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override { return concurrency::TaskType::gpu_draw; }

private:
    Profiler();

private:
    mutable bool m_show_demo_window = true;
    mutable bool m_show_another_window = false;

};

}

#endif
