#ifndef LEXGINE_CORE_CONCURRENCY_TASK_GRAPH_NODE_H

#include "lexgine/core/entity.h"
#include "lexgine/core/misc/optional.h"
#include "ring_buffer_task_queue.h"

#include <set>
#include <algorithm>
#include <atomic>

namespace lexgine {namespace core {namespace concurrency {

class TaskGraph;
class TaskSink;
class AbstractTask;

template<typename> class TaskGraphNodeAttorney;
template<typename> class TaskGraphNodeAttorney;

//! Implementation of task graph nodes
class TaskGraphNode
{
    friend class TaskGraphNodeAttorney<TaskGraph>;
    friend class TaskGraphNodeAttorney<TaskSink>;

public:
    using set_of_nodes_type = std::set<TaskGraphNode*>;

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

    /*! adds a task that depends on this task, i.e. provided task can only begin execution when this task is completed.
     Returns 'true' if the specified dependent task has been added successfully; returns 'false' if this dependent task has already been added to this node
    */
    bool addDependent(TaskGraphNode& task);    

    /*! adds dependency for this task graph node, i.e. the dependency is a task that must be completed BEFORE this task is run.
     Returns 'true' if provided dependency has been added for this node; returns 'false' if the dependency already exists.
    */
    bool addDependency(TaskGraphNode& task);

    //! retrieves index of the frame, to which this node belongs
    uint16_t frameIndex() const;

protected:
    void forceUndone();    //! forces the task to appear as uncompleted

private:
    TaskGraphNode(TaskGraphNode const& other);    //! NOTE: copies only identifier and the pointer to contained task but not completion status or dependency sets (see implementation)

private:
    uint64_t m_id;    //!< identifier of the node
    AbstractTask* m_contained_task;    //!< task contained by the node
    std::atomic_bool m_is_completed;    //!< equals 'true' if the task was completed. Equals 'false' otherwise
    bool m_is_scheduled;    //!< equals true if the node has already been scheduled, equals 'false' otherwise
    uint16_t m_frame_index;   //!< index of the frame, to which the task container belongs

    set_of_nodes_type m_dependencies;    //!< dependencies of this task. This task cannot run before all of its dependencies are executed
    set_of_nodes_type m_dependents;    //!< dependencies of this task. This task cannot be executed before the dependent tasks are completed
};

template<> class TaskGraphNodeAttorney<TaskGraph>
{
    friend class TaskGraph;

    static uint64_t getNodeId(TaskGraphNode const& parent_task_graph_node)
    {
        return parent_task_graph_node.m_id;
    }

    static inline TaskGraphNode cloneNodeForFrame(TaskGraphNode const& source_node, uint16_t frame_index)
    {
        TaskGraphNode rv{ source_node };
        rv.m_frame_index = frame_index;
        return rv;
    }

    static inline TaskGraphNode::set_of_nodes_type const& getDependents(TaskGraphNode const& parent_task_graph_node)
    {
        return parent_task_graph_node.m_dependents;
    }

    static inline TaskGraphNode::set_of_nodes_type const& getDependencies(TaskGraphNode const& parent_task_graph_node)
    {
        return parent_task_graph_node.m_dependencies;
    }

    static inline AbstractTask* getContainedTask(TaskGraphNode const& parent_task_graph_node)
    {
        return parent_task_graph_node.m_contained_task;
    }

    static inline void resetNodeCompletionStatus(TaskGraphNode& parent_task_graph_node)
    {
        parent_task_graph_node.forceUndone();
    }
};

template<> class TaskGraphNodeAttorney<TaskSink>
{
    friend class TaskSink;

private:
    static inline AbstractTask* getContainedTask(TaskGraphNode const& parent_task_graph_node)
    {
        return parent_task_graph_node.m_contained_task;
    }

    static inline void resetScheduleStatus(TaskGraphNode& parent_task_graph_node)
    {
        parent_task_graph_node.m_is_scheduled = false;
    }
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

}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH_NODE_H
#endif
