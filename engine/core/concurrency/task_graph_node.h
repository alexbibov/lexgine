#ifndef LEXGINE_CORE_CONCURRENCY_TASK_GRAPH_NODE_H
#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH_NODE_H

#include <set>

#include "engine/core/entity.h"
#include "engine/core/misc/optional.h"
#include "ring_buffer_task_queue.h"
#include "lexgine_core_concurrency_fwd.h"


namespace lexgine::core::concurrency {

//! Implementation of task graph nodes
class TaskGraphNode
{
public:
    using set_of_nodes = std::set<TaskGraphNode*>;

public:
    explicit TaskGraphNode(AbstractTask& task);    //! creates task graph node encapsulating given task

    TaskGraphNode(TaskGraphNode&& other);

    TaskGraphNode& operator=(TaskGraphNode const&) = delete;
    TaskGraphNode& operator=(TaskGraphNode&&) = default;

    virtual ~TaskGraphNode() = default;

    virtual bool operator==(TaskGraphNode const& other) const;

    /*! executes the task assigned to this node. This function returns 'true' if execution of the task has been 
     completed and the task is allowed to be removed from the execution queue. If the task has been rescheduled for
     later or repeated execution, the function then returns 'false'
    */
    bool execute(uint8_t worker_id);

    bool isCompleted() const;    //! returns 'true' if the task has been successfully completed. Returns 'false' if there was an error during execution or if the task was rescheduled

    void schedule(RingBufferTaskQueue<TaskGraphNode*>& queue);    //! schedules this task in the given queue and ensures that the task does not get scheduled twice

    bool isReadyToLaunch() const;    //! returns 'true' if all of this task's dependencies have been executed and the task is ready to launch

    bool isScheduled() const;    //! returns 'true' if the node has been scheduled

    /*! adds a task that depends on this task, i.e. provided task can only begin execution when this task is completed.
     Returns 'true' if the specified dependent task has been added successfully; returns 'false' if this dependent task has already been added to this node
    */
    bool addDependent(TaskGraphNode& task);    

    /*! adds dependency for this task graph node, i.e. the dependency is a task that must be completed BEFORE this task is run.
     Returns 'true' if provided dependency has been added for this node; returns 'false' if the dependency already exists.
    */
    bool addDependency(TaskGraphNode& task);

    //! creates task graph node with same id and containing the same task as this node, but having no dependents or dependency nodes.
    TaskGraphNode clone() const;

    //! returns identifier of the node
    uint64_t getId() const;

    //! Retrieves all dependencies of this node
    set_of_nodes getDependencies() const;

    //! Retrieves all dependent nodes of this node
    set_of_nodes getDependents() const;

    //! Resets execution status of the graph node, so it appears as undone
    void resetExecutionStatus();

    //! Retrieves task contained in the node
    AbstractTask* task() const;

    //! Defines custom user data associated with the task graph node
    void setUserData(uint64_t user_data);

    //! Retrieves custom user data associated with the task graph node
    uint64_t getUserData() const;

private:
    TaskGraphNode(TaskGraphNode const& other);    //! NOTE: copies only identifier and the pointer to contained task but not completion status or dependency sets (see implementation)

private:
    uint64_t m_id;    //!< identifier of the node
    uint64_t m_user_data;    //!< user data associated with the task graph node 
    AbstractTask* m_contained_task;    //!< task contained by the node
    std::atomic_bool m_is_completed;    //!< equals 'true' if the task was completed. Equals 'false' otherwise
    std::atomic_bool m_is_scheduled;    //!< equals true if the node has already been scheduled, equals 'false' otherwise

    set_of_nodes m_dependencies;    //!< dependencies of this task. This task cannot run before all of its dependencies are executed
    set_of_nodes m_dependents;    //!< dependencies of this task. This task cannot be executed before the dependent tasks are completed
};


class TaskGraphRootNode : public TaskGraphNode
{
public:
    explicit TaskGraphRootNode(AbstractTask& task);    //! creates root node wrapping provided task

    /*! Dummy implementation, always returns 'false' and causes assertion failure in debug mode.
     Root task graph nodes cannot have dependencies, therefore this function is provided in order to shadow
     the normal behavior of TaskGraphNode::addDependency(...)
    */
    bool addDependency(TaskGraphNode& task);    
};

}

#endif
