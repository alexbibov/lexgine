#ifndef LEXGINE_CORE_CONCURRENCY_TASK_H

#include "entity.h"
#include "class_names.h"
#include "optional.h"

#include <list>
#include <mutex>
#include <thread>

namespace lexgine {namespace core {namespace concurrency {

struct TaskExecutionStatistics final
{
    uint64_t execution_time;    //!< time of execution of the related task
};

//! Abstraction of a task that can be executed. This API as well as the inherited classes is OS-agnostic
class AbstractTask : public NamedEntity<class_names::Task>
{
public:
    AbstractTask();
    AbstractTask(std::list<AbstractTask const*> const& dependencies);

    void execute();    //! executes the task on the calling thread

    bool isCompleted() const;    //! returns 'true' if the task has been successfully completed. Returns 'false' otherwise
    TaskExecutionStatistics const& getExecutionStatistics() const;    //! returns execution statistics of the task

private:
    std::list<AbstractTask const*> m_dependencies;    //!< dependencies of this task. This task cannot be executed before the dependent tasks are completed.
    bool m_is_completed;    //!< equals 'true' if the task was completed. Equals 'false' otherwise.
    mutable std::mutex m_execution_mutex;    //!< mutex, which gets locked while the task is being executed
    TaskExecutionStatistics m_execution_statistics;    //!< execution statistics of the task

    virtual void do_task() = 0;    //!< runs the actual workload related to the task
};


// Abstraction of a task with certain return value
template<typename T>
class AbstractReturningTask : public AbstractTask
{
public:
    AbstractReturningTask():
        AbstractTask()
    {

    }

    AbstractReturningTask(std::list<AbstractTask const*> const& dependencies):
        AbstractTask{ dependencies }
    {

    }

    //! returns the output value written by the task. If the task has not yet been completed returns a NULL Optional
    misc::Optional<T> const& getReturnValue() const
    {
        return isCompleted() ? m_return_value : Optional{};
    }

protected:
    //! sets new return value for the task. Note that this should only be done during overridden call of do_task() to avoid race conditions
    void setReturnValue(T const& return_value)
    {
        m_return_value = return_value;
    }

private:
    misc::Optional<T> m_return_value;    //!< return value of the task
};


}

}}

#define LEXGINE_CORE_CONCURRENCY_TASK_H
#endif
