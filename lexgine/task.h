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


//! task type enumeration
enum class TaskType
{
    cpu,
    gpu_draw,
    gpu_compute,
    gpu_copy,
    exit,
    other
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

    AbstractTask(AbstractTask const&) = delete;
    AbstractTask(AbstractTask&&) = delete;

    ~AbstractTask() = default;

    AbstractTask& operator=(AbstractTask const&) = delete;
    AbstractTask& operator=(AbstractTask&&) = delete;

    void executeAsync(uint8_t worker_id);    //! executes the task asynchronously

    bool isCompleted() const;    //! returns 'true' if the task has been successfully completed. Returns 'false' otherwise
    TaskExecutionStatistics const& getExecutionStatistics() const;    //! returns execution statistics of the task
    bool isReadyToLaunch() const;    //! returns 'true' if all of this task's dependencies have been executed and the task is ready to launch

    void addDependent(AbstractTask& task);    //! adds task that depends on this task, i.e. provided task can only begin execution when this task is completed

    bool hasDependents() const;    //! returns 'true' if the task has dependent tasks, returns 'false' otherwise

private:
    virtual void do_task(uint8_t worker_id) = 0;    //! runs the actual workload related to the task
    virtual TaskType get_task_type() const = 0;    //! returns type of the task
    std::list<AbstractTask*> const& dependents() const;    //! returns list of dependent tasks

    std::list<AbstractTask*> m_dependencies;    //!< dependencies of this task. This task cannot run before all of its dependencies are executed
    std::list<AbstractTask*> m_dependents;    //!< dependencies of this task. This task cannot be executed before the dependent tasks are completed
    bool m_is_completed;    //!< equals 'true' if the task was completed. Equals 'false' otherwise
    TaskExecutionStatistics m_execution_statistics;    //!< execution statistics of the task

    bool m_visit_flag;    //!< determines, whether the task node has been visited during task graph traversal
};



template<> class AbstractTaskAttorney<TaskGraph>
{
public:
    static inline std::list<AbstractTask*> const& getDependentTasks(AbstractTask const& parent_task)
    {
        return parent_task.dependents();
    }

    static inline bool visited(AbstractTask const& parent_task)
    {
        return parent_task.m_visit_flag;
    }

    static inline void setVisitFlag(AbstractTask& parent_task, bool visit_flag_value)
    {
        parent_task.m_visit_flag = visit_flag_value;
    }

    static inline TaskType getTaskType(AbstractTask& parent_task)
    {
        return parent_task.get_task_type();
    }
};


}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_H
#endif
