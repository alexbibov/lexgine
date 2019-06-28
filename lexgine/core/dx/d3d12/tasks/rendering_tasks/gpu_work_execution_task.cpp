#include "lexgine/core/dx/d3d12/device.h"

#include "gpu_work_execution_task.h"


using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;

GpuWorkExecutionTask::GpuWorkExecutionTask(Device& device, FrameProgressTracker const& frame_progress_tracker, 
    BasicRenderingServices const& basic_rendering_services, GpuWorkType work_type)
    : m_device{ device }
    , m_frame_progress_tracker{ &frame_progress_tracker }
    , m_gpu_work_type{ work_type }
{
}

GpuWorkExecutionTask::GpuWorkExecutionTask(Device& device, BasicRenderingServices const& basic_rendering_services, GpuWorkType work_type)
    : m_device{ device }
    , m_gpu_work_type{ work_type }
{
}