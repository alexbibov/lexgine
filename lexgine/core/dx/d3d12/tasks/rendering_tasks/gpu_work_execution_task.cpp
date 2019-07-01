#include "lexgine/core/dx/d3d12/device.h"

#include "gpu_work_execution_task.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;

namespace {

CommandType commandTypeFromGPUWorkType(GpuWorkType gpu_work_type)
{
    switch (gpu_work_type)
    {
    case GpuWorkType::graphics:
        return CommandType::direct;

    case GpuWorkType::compute:
        return CommandType::compute;

    case GpuWorkType::copy:
        return CommandType::copy;

    default:
        __assume(0);
    }
}

}

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

void GpuWorkExecutionTask::addSource(GpuWorkSource& source)
{
    CommandList& cmd_list = source.gpuWorkPackage();
    m_gpu_work_sources.push_back(&cmd_list);
}

bool GpuWorkExecutionTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    switch (m_gpu_work_type)
    {
    case GpuWorkType::graphics:
        m_device.defaultCommandQueue().executeCommandLists(m_gpu_work_sources.data(), m_gpu_work_sources.size());
        break;

    case GpuWorkType::compute:
        break;

    case GpuWorkType::copy:
        break;

    default:
        __assume(0);
    }

    return true;
}

concurrency::TaskType GpuWorkExecutionTask::type() const
{
    switch (m_gpu_work_type)
    {
    case GpuWorkType::graphics:
        return concurrency::TaskType::gpu_draw;
    case GpuWorkType::compute:
        return concurrency::TaskType::gpu_compute;
    case GpuWorkType::copy:
        return concurrency::TaskType::gpu_copy;
    default:
        __assume(0);
    }
}

GpuWorkSource::GpuWorkSource(Device& device, GpuWorkType work_type)
    : m_cmd_list{ device.createCommandList(commandTypeFromGPUWorkType(work_type), 0x1) }
{
}
