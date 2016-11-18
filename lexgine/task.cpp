#include "task.h"

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
    do_task();
    m_is_completed = true;
}

void AbstractTask::execute_async()
{
    m_execution_mutex.lock();
    std::thread{ &AbstractTask::execute }.detach();
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
