#ifndef LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_GPU_PROFILING_QUERIES_FLUSH_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_GPU_PROFILING_QUERIES_FLUSH_TASK_H

#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/concurrency/schedulable_task.h"
#include "rendering_work.h"

namespace lexgine::core::dx::d3d12::tasks::rendering_tasks {

class GpuProfilingQueriesFlushTask final : public RenderingWork
{
public:
    void updateRenderingConfiguration(RenderingConfigurationUpdateFlags update_flags, RenderingConfiguration const& rendering_configuration) override {}
    static std::shared_ptr<GpuProfilingQueriesFlushTask> create(Globals& globals);

public:    // required by AbstractTask
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override { return concurrency::TaskType::gpu_draw; }

private:
    GpuProfilingQueriesFlushTask(Globals& globals);

private:
    CommandList* m_cmd_list_ptr = nullptr;
};

}

#endif
