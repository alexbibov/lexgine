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
    AbstractTask();
    AbstractTask(std::list<AbstractTask*> const& dependencies);

    void executeAsync(uint8_t worker_id);    //! executes the task asynchronously

    bool isCompleted() const;    //! returns 'true' if the task has been successfully completed. Returns 'false' otherwise
    TaskExecutionStatistics const& getExecutionStatistics() const;    //! returns execution statistics of the task

    void addDependent(AbstractTask& task);    //! adds task that depends on this task, i.e. it can only begin execution when this task is completed


    //! sets inputs for the task
    template<typename ... Args>
    void setInputs(Args... args)
    {

    }

protected:
    template<typename... Args> class TaskInputs;    //! describes generic task inputs
    template<> class TaskInputs<> {};
    template<typename T, typename... Rest>
    class TaskInputs<T, Rest...> : public TaskInputs<Rest...>
    {
    protected:
        template<uint32_t id>
        struct input_data_type_selector
        {
            using value_type = typename TaskInputs<Rest...>:: template input_data_type_selector<id - 1>::value_type;
        };

        template<>
        struct input_data_type_selector<0U>
        {
            using value_type = T;
        };

        template<uint32_t id>
        using get_value_type_by_input_id = typename input_data_type_selector<id>::value_type;


    public:
        template<uint32_t id>
        get_value_type_by_input_id<id> const& get() const
        {
            return TaskInputs<Rest...>::get();
        }

        template<>
        T const& get<0>() const
        {
            return data;
        }


        // NOTE TO MYSELF: continue implementation of task inputs abstraction


    protected:
        T data;
    };

private:
    virtual void do_task(uint8_t worker_id) = 0;    //!< runs the actual workload related to the task
    std::list<AbstractTask*> const& dependents() const;    //!< returns list of dependent tasks

    std::list<AbstractTask*> m_dependents;    //!< dependencies of this task. This task cannot be executed before the dependent tasks are completed.
    bool m_is_completed;    //!< equals 'true' if the task was completed. Equals 'false' otherwise.
    mutable std::mutex m_execution_mutex;    //!< mutex, which gets locked while the task is being executed
    TaskExecutionStatistics m_execution_statistics;    //!< execution statistics of the task
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

    AbstractReturningTask(std::list<AbstractTask*> const& dependents):
        AbstractTask{ dependents }
    {

    }

    //! Returns result of the task execution
    misc::Optional<T> getResult() const
    {
        return m_return_value;
    }

protected:
    //! Sets result of the task execution. This function is to be called by derived implementations
    void setReturnValue(T const& return_value)
    {
        m_return_value = return_value;
    }


private:
    misc::Optional<T> m_return_value;    //!< return value of the task
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
