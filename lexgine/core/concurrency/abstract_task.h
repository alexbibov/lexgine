#ifndef LEXGINE_CORE_CONCURRENCY_ABSTRACT_TASK_H
#define LEXGINE_CORE_CONCURRENCY_ABSTRACT_TASK_H

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/concurrency/lexgine_core_concurrency_fwd.h"

namespace lexgine::core::concurrency {

struct TaskExecutionStatistics final
{
    uint8_t worker_id;    //!< identifier of the worker thread that has executed the task. Primarily for debug purposes.
    uint64_t execution_time;    //!< time of execution of the related task provided in milliseconds.
};

//! task type enumeration
enum class TaskType
{
    cpu,
    gpu_draw,
    gpu_compute,
    gpu_copy,
    other
};

template<typename T> class AbstractTaskAttorney;

class AbstractTask : public NamedEntity<class_names::Task>
{
    friend class AbstractTaskAttorney<TaskGraph>;
public:
    AbstractTask(std::string const& debug_name = "", bool expose_in_task_graph = true);
    AbstractTask(AbstractTask const&) = delete;    // copying tasks doesn't make much sense and complicates things

    // moving ownership of tasks is not allowed either since task graph nodes refer to their corresponding
    // tasks using pointers. Therefore, if moving was allowed it would have been possible for task ownership to
    // get moved to new containing object, while the task graph node was still referring to the old object
    AbstractTask(AbstractTask&&) = delete;    

    virtual ~AbstractTask() = default;
    AbstractTask& operator=(AbstractTask const&) = delete;
    AbstractTask& operator=(AbstractTask&&) = delete;

    TaskExecutionStatistics const& getExecutionStatistics() const;    //! returns execution statistics of the task

    bool execute(uint8_t worker_id, uint64_t user_data);    //! executes the task and returns 'true' if the task has been completed or 'false' if it has to be rescheduled to be executed later

public:
    /*! Calls the actual implementation of the task.
     @param worker_id provides identifier of the worker thread, which executes the task;
     @param user_data is sets the custom user data to be forwarded to the task.
     
     The function must return 'true' if the execution of the task should be marked as "successful" and the task is to be removed from the execution
     queue. 
     
     If the function returns 'false', the task will not be removed from the execution queue and another attempt to execute the task will
     be made at some later time.
    */
    virtual bool doTask(uint8_t worker_id, uint64_t user_data) = 0;

    virtual TaskType type() const = 0;    //! returns type of the task

private:
    TaskExecutionStatistics m_execution_statistics;    //!< execution statistics of the task
    bool m_exposed_in_task_graph;    //!< 'true' if the task should be included into DOT representation of the task graph for debugging purposes, 'false' otherwise. Default is 'true'.
};

template<> class AbstractTaskAttorney<TaskGraph>
{
    friend class TaskGraph;

    static bool isTaskExposedInDebugInformation(AbstractTask const& parent_task)
    {
        return parent_task.m_exposed_in_task_graph;
    }
};

}

#endif
