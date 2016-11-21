#include "task.h"

#include <chrono>

using namespace lexgine::core::concurrency;

AbstractTask::AbstractTask():
    m_is_completed{ false }
{
}

AbstractTask::AbstractTask(std::list<AbstractTask const*> const & dependencies):
    m_dependencies{ dependencies },
    m_is_completed{ false }
{
}

void AbstractTask::execute()
{
    auto task_begin_execution_time_point = std::chrono::system_clock::now();
    do_task();
    auto task_complete_execution_time_point = std::chrono::system_clock::now();
    m_is_completed = true;

    uint64_t completion_time_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(task_complete_execution_time_point - task_begin_execution_time_point).count();
    m_completion_callback(completion_time_in_ms);
}

void AbstractTask::execute_async()
{
    m_execution_mutex.lock();
    std::thread{ &AbstractTask::execute, this }.detach();
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

void AbstractTask::setCompletionCallback(std::function<void(uint64_t completion_time)> const & callback)
{
    m_completion_callback = callback;
}
