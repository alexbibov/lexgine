#ifndef LEXGINE_CORE_CONCURRENCY_ABSTRACT_TASK_H
#include "entity.h"
#include "class_names.h"

namespace lexgine { namespace core { namespace concurrency {

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

template<typename> class AbstractTaskAttorney;
class TaskGraph;

class AbstractTask : public NamedEntity<class_names::Task>
{
    friend class AbstractTaskAttorney<TaskGraph>;
public:
    AbstractTask(std::string const& debug_name = "", bool expose_in_task_graph = true);
    AbstractTask(AbstractTask const&) = delete;
    AbstractTask(AbstractTask&&) = delete;
    ~AbstractTask() = default;
    AbstractTask& operator=(AbstractTask const&) = delete;
    AbstractTask& operator=(AbstractTask&&) = delete;

    TaskExecutionStatistics const& getExecutionStatistics() const;    //! returns execution statistics of the task

    bool execute(uint8_t worker_id, uint16_t frame_index);    //! executes the task and returns 'true' if the task has been completed or 'false' if it has to be rescheduled to be executed later

private:
    /*! calls actual implementation of the task, worker_id provides identifier of the worker thread, which executes the task; frame_index is 
     a nominal index of the frame being composed. This index can be used by task implementation to organize exclusive access to shared data.
     The function must return 'true' if execution of the task should be marked as "successful" and the task should be removed from the execution
     queue. If the function returns 'false', the task will not be removed from the execution queue and another attempt to execute the task will
     be made at some later time.
    */
    virtual bool do_task(uint8_t worker_id, uint16_t frame_index) = 0;
    virtual TaskType get_type() const = 0;    //! returns type of the task

    TaskExecutionStatistics m_execution_statistics;    //!< execution statistics of the task
    bool m_exposed_in_task_graph;    //!< 'true' if the task should be included into DOT representation of the task graph for debugging purposes, 'false' otherwise. Default is 'true'.
};


template<> class AbstractTaskAttorney<TaskGraph>
{
    friend class TaskGraph;

private:
    static inline TaskType getTaskType(AbstractTask const& parent_abstract_task)
    {
        return parent_abstract_task.get_type();
    }

    static inline bool isExposedInTaskGraph(AbstractTask const& parent_abstract_task)
    {
        return parent_abstract_task.m_exposed_in_task_graph;
    }
};


}}}

#define LEXGINE_CORE_CONCURRENCY_ABSTRACT_TASK_H
#endif
