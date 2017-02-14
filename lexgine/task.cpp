#include "task.h"

#include <chrono>

using namespace lexgine::core::concurrency;

AbstractTask::AbstractTask(std::string const& debug_name):
    m_is_completed{ false },
    m_visit_flag{ false }
{
    setStringName(debug_name);
}

AbstractTask::AbstractTask(std::list<AbstractTask*> const & dependencies, std::string const& debug_name):
    m_dependents{ dependencies },
    m_is_completed{ false },
    m_visit_flag{ false }
{
    setStringName(debug_name);
}


void AbstractTask::executeAsync(uint8_t worker_id)
{
    auto task_begin_execution_time_point = std::chrono::system_clock::now();
    do_task(worker_id);
    auto task_complete_execution_time_point = std::chrono::system_clock::now();
    m_is_completed = true;

    uint64_t completion_time_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(task_complete_execution_time_point - task_begin_execution_time_point).count();

    m_execution_statistics.worker_id = worker_id;
    m_execution_statistics.execution_time = completion_time_in_ms;
}

bool lexgine::core::concurrency::AbstractTask::isCompleted() const
{
    return m_is_completed;
}

TaskExecutionStatistics const& AbstractTask::getExecutionStatistics() const
{
    return m_execution_statistics;
}

bool AbstractTask::isReadyToLaunch() const
{
    for (AbstractTask const* dependency : m_dependencies)
    {
        if (!dependency->isCompleted()) return false;
    }

    return true;
}

void AbstractTask::addDependent(AbstractTask& task)
{
    m_dependents.push_back(&task);
    task.m_dependencies.push_back(this);
}

bool AbstractTask::hasDependents() const
{
    return m_dependents.size() > 0;
}

std::list<AbstractTask*> const& AbstractTask::dependents() const
{
    return m_dependents;
}
