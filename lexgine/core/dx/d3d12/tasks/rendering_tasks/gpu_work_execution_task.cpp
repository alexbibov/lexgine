#include <numeric>

#include "lexgine/core/exception.h"
#include "lexgine/core/dx/d3d12/device.h"

#include "rendering_work.h"
#include "gpu_work_execution_task.h"


using namespace lexgine::core;
using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;


std::shared_ptr<GpuWorkExecutionTask> GpuWorkExecutionTask::create(
    Device& device, std::string const& debug_name,
    BasicRenderingServices const& basic_rendering_services, bool enable_profiling/* = true*/)
{
    return std::shared_ptr<GpuWorkExecutionTask>{ new GpuWorkExecutionTask{ device, debug_name, basic_rendering_services, enable_profiling } };
}

GpuWorkExecutionTask::GpuWorkExecutionTask(Device& device,
    std::string const& debug_name, BasicRenderingServices const& basic_rendering_services, bool enable_profiling/* = true*/)
    : SchedulableTask{ debug_name }
    , m_device{ device }
{

}

void GpuWorkExecutionTask::addSource(RenderingWork& source)
{
    assert(m_gpu_work_sources.empty()
        || RenderingWorkAttorney<GpuWorkExecutionTask>::renderingWorkCommandType(*m_gpu_work_sources.back())
        == RenderingWorkAttorney<GpuWorkExecutionTask>::renderingWorkCommandType(source));
    m_gpu_work_sources.push_back(&source);
}

bool GpuWorkExecutionTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    if (m_gpu_work_sources.empty()) return true;

    if (m_packed_gpu_work.empty())    // prepare GPU work package on first invocation
    {
        size_t total_command_list_count = std::accumulate(m_gpu_work_sources.begin(),
            m_gpu_work_sources.end(), 0ULL,
            [](size_t current, RenderingWork* next)
            {
                return current
                    + RenderingWorkAttorney<GpuWorkExecutionTask>::renderingWorkCommands(*next).size();
            }
        );

        if (total_command_list_count > 32)
        {
            logger().out("GPU work package " + getStringName() + " contains over 32 command lists, which may "
                "lead to performance degradation. Consider refactoring", LogMessageType::exclamation);
        }

        m_packed_gpu_work.reserve(total_command_list_count);
        for (RenderingWork* work : m_gpu_work_sources)
        {
            for (auto& cmd_list : RenderingWorkAttorney<GpuWorkExecutionTask>::renderingWorkCommands(*work))
                m_packed_gpu_work.push_back(&cmd_list);
        }
    }

    switch (RenderingWorkAttorney<GpuWorkExecutionTask>::renderingWorkCommandType(*m_gpu_work_sources.back()))
    {
    case CommandType::direct:
        m_device.defaultCommandQueue().executeCommandLists(m_packed_gpu_work.data(), m_gpu_work_sources.size());
        break;

    case CommandType::compute:
        m_device.asyncCommandQueue().executeCommandLists(m_packed_gpu_work.data(), m_gpu_work_sources.size());
        break;

    case CommandType::copy:
        m_device.copyCommandQueue().executeCommandLists(m_packed_gpu_work.data(), m_gpu_work_sources.size());
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

    switch (RenderingWorkAttorney<GpuWorkExecutionTask>::renderingWorkCommandType(*m_gpu_work_sources.back()))
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
