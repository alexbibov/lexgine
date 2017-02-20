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
    exit,
    other
};

template<typename> class AbstractTaskAttorney;
class TaskGraph;

class AbstractTask : public NamedEntity<class_names::Task>
{
    friend class AbstractTaskAttorney<TaskGraph>;
public:
    AbstractTask(std::string const& debug_name = "");
    AbstractTask(AbstractTask const&) = delete;
    AbstractTask(AbstractTask&&) = delete;
    ~AbstractTask() = default;
    AbstractTask& operator=(AbstractTask const&) = delete;
    AbstractTask& operator=(AbstractTask&&) = delete;

    TaskExecutionStatistics const& getExecutionStatistics() const;    //! returns execution statistics of the task
    virtual bool allowsConcurrentExecution() const = 0;    //! returns 'true' if implementation of this task provides thread-safe execute(...) procedure. Returns 'false' otherwise.

    bool execute(uint8_t worker_id, uint16_t frame_index);    //! executes the task and returns 'true' on success and 'false' on failure

private:
    virtual bool do_task(uint8_t worker_id, uint16_t frame_index) = 0;    //! calls actual implementation of the task, worker_id provides identifier of the worker thread, which executes the task
    virtual TaskType get_type() const = 0;    //! returns type of the task

    TaskExecutionStatistics m_execution_statistics;    //!< execution statistics of the task
};


template<> class AbstractTaskAttorney<TaskGraph>
{
    friend class TaskGraph;

private:
    static inline TaskType getTaskType(AbstractTask const& parent_abstract_task)
    {
        return parent_abstract_task.get_type();
    }
};


}}}

#define LEXGINE_CORE_CONCURRENCY_ABSTRACT_TASK_H
#endif
