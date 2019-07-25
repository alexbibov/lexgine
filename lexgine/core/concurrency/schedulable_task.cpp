#include "schedulable_task.h"

using namespace lexgine::core;
using namespace lexgine::core::concurrency;

SchedulableTask::SchedulableTask(std::string const& debug_name, bool expose_in_task_graph, std::unique_ptr<ProfilingService>&& profiling_service) :
    AbstractTask{ debug_name, expose_in_task_graph, std::move(profiling_service) },
    TaskGraphNode{ static_cast<AbstractTask&>(*this) }
{
}

SchedulableTaskWithoutConcurrency::SchedulableTaskWithoutConcurrency(std::string const& debug_name, bool expose_in_task_graph, std::unique_ptr<ProfilingService>&& profiling_service) :
    SchedulableTask{ debug_name, expose_in_task_graph, std::move(profiling_service) }
{
}

bool SchedulableTaskWithoutConcurrency::execute(uint8_t worker_id, uint16_t frame_index)
{
    if (m_task_execution_mutex.try_lock())
    {
        bool rv = AbstractTask::execute(worker_id, frame_index);
        m_task_execution_mutex.unlock();
        return rv;
    }

    return false;
}



bool lexgine::core::concurrency::checkConcurrentExecutionAbility(AbstractTask const& task)
{
    return dynamic_cast<SchedulableTaskWithoutConcurrency const*>(&task) == nullptr;
}

RootSchedulableTask::RootSchedulableTask(std::string const& debug_name, std::unique_ptr<ProfilingService>&& profiling_service) :
    AbstractTask{ debug_name, true, std::move(profiling_service) },
    TaskGraphRootNode{ static_cast<AbstractTask&>(*this) }
{
}
