#ifndef LEXGINE_CORE_DX_D3D12_TASKS_UI_DRAW_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_UI_DRAW_TASK_H

#include "lexgine/osinteraction/windows/lexgine_osinteraction_windows_fwd.h"
#include "lexgine/core/concurrency/schedulable_task.h"

namespace lexgine::core::dx::d3d12::tasks {

class UIDrawTask final : public concurrency::SchedulableTask
{
public:
    UIDrawTask(osinteraction::windows::Window const& window);
    UIDrawTask(UIDrawTask const&) = delete;

    UIDrawTask& operator=(UIDrawTask const&) = delete;

public:    // AbstractTask interface implementation
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override;

private:
};

}

#endif

