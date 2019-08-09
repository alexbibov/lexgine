#ifndef LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_GPU_WORK_EXECUTION_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_GPU_WORK_EXECUTION_TASK_H

#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/command_list.h"
#include "lexgine/core/concurrency/schedulable_task.h"

#include "lexgine_core_dx_d3d12_tasks_rendering_tasks_fwd.h"

namespace lexgine::core::dx::d3d12::tasks::rendering_tasks {

class GpuWorkExecutionTask : public concurrency::SchedulableTask
{
    using Entity::getId;

public:

    static std::shared_ptr<GpuWorkExecutionTask> create(
        Device& device, std::string const& debug_name,
        BasicRenderingServices const& basic_rendering_services, bool enable_profiling = true);

    void addSource(RenderingWork& source);

private:

    GpuWorkExecutionTask(Device& device,
        std::string const& debug_name, BasicRenderingServices const& basic_rendering_services,
        bool enable_profiling = true);

private:    // required by AbstractTask interface
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override;

private:
    Device& m_device;
    std::list<RenderingWork*> m_gpu_work_sources;
    std::vector<CommandList*> m_packed_gpu_work;
};

}

#endif
