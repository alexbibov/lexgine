#ifndef LEXGINE_CORE_CONCURRENCY_TASK_H

#include "entity.h"
#include "class_names.h"
#include "optional.h"

#include <list>

namespace lexgine {namespace core {namespace concurrency {


struct TaskExecutionStatistics final
{
    uint8_t worker_id;    //!< identifier of the worker thread that has executed the task. Primarily for debug purposes.
    uint64_t execution_time;    //!< time of execution of the related task
};

template<typename T> class AbstractTaskAttorney;
class TaskGraph;

//! Abstraction of a task that can be executed. This API as well as the inherited classes is OS-agnostic
class AbstractTask : public NamedEntity<class_names::Task>
{
    friend class AbstractTaskAttorney<TaskGraph>;

public:
    AbstractTask(std::string const& debug_name = "");
    AbstractTask(std::list<AbstractTask*> const& dependencies, std::string const& debug_name = "");

    void executeAsync(uint8_t worker_id);    //! executes the task asynchronously

    bool isCompleted() const;    //! returns 'true' if the task has been successfully completed. Returns 'false' otherwise
    TaskExecutionStatistics const& getExecutionStatistics() const;    //! returns execution statistics of the task
    bool isReadyToLaunch() const;    //! returns 'true' if all of this task's dependencies have been executed and the task is ready to launch

    void addDependent(AbstractTask& task);    //! adds task that depends on this task, i.e. provided task can only begin execution when this task is completed

    void setDebugName(std::string const& debug_name);    //! assigns user-readable debug name to the task
    std::string getDebugName() const;    //! returns user-readable debug name previously assigned to this task

private:
    virtual void do_task(uint8_t worker_id) = 0;    //!< runs the actual workload related to the task
    std::list<AbstractTask*> const& dependents() const;    //!< returns list of dependent tasks

    std::list<AbstractTask*> m_dependencies;    //!< dependencies of this task. This task cannot run before all of its dependencies are executed
    std::list<AbstractTask*> m_dependents;    //!< dependencies of this task. This task cannot be executed before the dependent tasks are completed
    bool m_is_completed;    //!< equals 'true' if the task was completed. Equals 'false' otherwise
    TaskExecutionStatistics m_execution_statistics;    //!< execution statistics of the task
    std::string m_debug_name;    //!< user-readable name of the task, which can be used for debugging purposes
};



template<> class AbstractTaskAttorney<TaskGraph>
{
public:
    inline std::list<AbstractTask*> const& getDependentTasks(AbstractTask const& parent_task)
    {
        return parent_task.dependents();
    }
};


}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_H
#endif
