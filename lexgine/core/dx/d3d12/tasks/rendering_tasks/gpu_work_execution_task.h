#ifndef LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_GPU_WORK_EXECUTION_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_GPU_WORK_EXECUTION_TASK_H

#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/command_list.h"
#include "lexgine/core/concurrency/schedulable_task.h"

namespace lexgine::core::dx::d3d12::tasks::rendering_tasks {

enum class GpuWorkType
{
    graphics,
    compute,
    copy
};

class GpuWorkSource
{
public:
    GpuWorkSource(Device& device, GpuWorkType work_type);
    CommandList& gpuWorkPackage() { return m_cmd_list; }

private:
    CommandList m_cmd_list;
};

class GpuWorkExecutionTask : public concurrency::SchedulableTask
{
public:
    GpuWorkExecutionTask(Device& device, FrameProgressTracker const& frame_progress_tracker, 
        BasicRenderingServices const& basic_rendering_services, GpuWorkType work_type);
    GpuWorkExecutionTask(Device& device, BasicRenderingServices const& basic_rendering_services, GpuWorkType work_type);
    
    void addSource(GpuWorkSource& source);

private:    // required by AbstractTask interface
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override;

private:
    Device& m_device;
    FrameProgressTracker const* m_frame_progress_tracker = nullptr;
    GpuWorkType m_gpu_work_type;
    std::vector<CommandList*> m_gpu_work_sources;
};

}

#endif
