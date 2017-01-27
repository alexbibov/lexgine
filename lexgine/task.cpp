#include "task.h"

#include <chrono>

using namespace lexgine::core::concurrency;

AbstractTask::AbstractTask():
    m_is_completed{ false }
{
}

AbstractTask::AbstractTask(std::list<AbstractTask*> const & dependencies):
    m_dependents{ dependencies },
    m_is_completed{ false }
{
}


void AbstractTask::executeAsync(uint8_t worker_id)
{
    m_execution_mutex.lock();

    auto task_begin_execution_time_point = std::chrono::system_clock::now();
    do_task(worker_id);
    auto task_complete_execution_time_point = std::chrono::system_clock::now();
    m_is_completed = true;

    uint64_t completion_time_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(task_complete_execution_time_point - task_begin_execution_time_point).count();

    m_execution_statistics.worker_id = worker_id;
    m_execution_statistics.execution_time = completion_time_in_ms;

    m_execution_mutex.unlock();
}

bool lexgine::core::concurrency::AbstractTask::isCompleted() const
{
    if (m_execution_mutex.try_lock())
    {
        bool rv = m_is_completed;
        m_execution_mutex.unlock();
        return rv;
    }
    return false;
}

TaskExecutionStatistics const& AbstractTask::getExecutionStatistics() const
{
    return m_execution_statistics;
}

void AbstractTask::addDependent(AbstractTask& task)
{
    m_dependents.push_back(&task);
}

std::list<AbstractTask*> const& AbstractTask::dependents() const
{
    return m_dependents;
}
