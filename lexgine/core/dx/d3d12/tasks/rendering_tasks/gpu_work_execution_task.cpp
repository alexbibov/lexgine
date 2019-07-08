#include "lexgine/core/exception.h"
#include "lexgine/core/dx/d3d12/device.h"
#include "gpu_work_execution_task.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;

GpuWorkExecutionTask::GpuWorkExecutionTask(Device& device, FrameProgressTracker const& frame_progress_tracker, 
    BasicRenderingServices const& basic_rendering_services)
    : m_device{ device }
    , m_frame_progress_tracker{ &frame_progress_tracker }
{
}

GpuWorkExecutionTask::GpuWorkExecutionTask(Device& device, BasicRenderingServices const& basic_rendering_services)
    : m_device{ device }
{
}

std::shared_ptr<GpuWorkExecutionTask> GpuWorkExecutionTask::create(Device& device, 
    FrameProgressTracker const& frame_progress_tracker, 
    BasicRenderingServices const& basic_rendering_services)
{
    return std::shared_ptr<GpuWorkExecutionTask>{ new GpuWorkExecutionTask{ device, frame_progress_tracker, basic_rendering_services } };
}

std::shared_ptr<GpuWorkExecutionTask> GpuWorkExecutionTask::create(Device& device, 
    BasicRenderingServices const& basic_rendering_services)
{
    return std::shared_ptr<GpuWorkExecutionTask>{ new GpuWorkExecutionTask{ device, basic_rendering_services } };
}

void GpuWorkExecutionTask::addSource(GpuWorkSource& source)
{
    CommandList& cmd_list = source.gpuWorkPackage();
    assert(m_gpu_work_sources.empty() 
        || m_gpu_work_sources[0]->commandType() == cmd_list.commandType());

    m_gpu_work_sources.push_back(&cmd_list);
}

bool GpuWorkExecutionTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    if (m_gpu_work_sources.empty()) return true;

    switch (m_gpu_work_sources[0]->commandType())
    {
    case CommandType::direct:
        m_device.defaultCommandQueue().executeCommandLists(m_gpu_work_sources.data(), m_gpu_work_sources.size());
        break;

    case CommandType::compute:
        m_device.asyncCommandQueue().executeCommandLists(m_gpu_work_sources.data(), m_gpu_work_sources.size());
        break;

    case CommandType::copy:
        m_device.copyCommandQueue().executeCommandLists(m_gpu_work_sources.data(), m_gpu_work_sources.size());
        break;

    case CommandType::bundle:
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Command bundles cannot be added as GPU work packages");

    default:
        __assume(0);
    }

    return true;
}

concurrency::TaskType GpuWorkExecutionTask::type() const
{
    if (m_gpu_work_sources.empty()) return concurrency::TaskType::other;

    switch (m_gpu_work_sources[0]->commandType())
    {
    case CommandType::direct:
        return concurrency::TaskType::gpu_draw;
    case CommandType::compute:
        return concurrency::TaskType::gpu_compute;
    case CommandType::copy:
        return concurrency::TaskType::gpu_copy;

    default:
        __assume(0);
    }
}

GpuWorkSource::GpuWorkSource(Device& device, CommandType work_type)
    : m_cmd_list{ device.createCommandList(work_type, 0x1) }
{
}
