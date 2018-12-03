#include "schedulable_task.h"

using namespace lexgine::core::concurrency;

SchedulableTask::SchedulableTask(std::string const& debug_name, bool expose_in_task_graph):
    AbstractTask{ debug_name, expose_in_task_graph },
    TaskGraphNode{ static_cast<AbstractTask&>(*this) }
{
}

SchedulableTaskWithoutConcurrency::SchedulableTaskWithoutConcurrency(std::string const& debug_name, bool expose_in_task_graph):
    SchedulableTask{ debug_name, expose_in_task_graph }
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



bool lexgine::core::concurrency::checkConcurrentExecutionAbility(AbstractTask const & task)
{
    return dynamic_cast<SchedulableTaskWithoutConcurrency const*>(&task) == nullptr;
}

RootSchedulableTask::RootSchedulableTask(std::string const& debug_name):
    AbstractTask{ debug_name },
    TaskGraphRootNode{ static_cast<AbstractTask&>(*this) }
{
}
