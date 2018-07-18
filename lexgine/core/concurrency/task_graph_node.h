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
    TaskGraphNode(AbstractTask& task);    //! creates task graph node encapsulating given task
    TaskGraphNode(AbstractTask&task, uint32_t id);    //! creates task graph node, which encapsulates the given task and has provided identifier
    TaskGraphNode(TaskGraphNode const& other) = delete;
    TaskGraphNode(TaskGraphNode&&) = default;

    TaskGraphNode& operator=(TaskGraphNode const&) = delete;
    TaskGraphNode& operator=(TaskGraphNode&&) = default;

    virtual ~TaskGraphNode() = default;

    /*! executes the task assigned to this node. This function returns 'true' if execution of the task has been 
     completed and the task is allowed to be removed from the execution queue. If the task has been rescheduled for
     later or repeated execution, the function then returns 'false'
    */
    bool execute(uint8_t worker_id);

    bool isCompleted() const;    //! returns 'true' if the task has been successfully completed. Returns 'false' if there was an error during execution or if the task was rescheduled

    void schedule(RingBufferTaskQueue<TaskGraphNode*>& queue);    //! schedules this task in the given queue and ensures that the task does not get scheduled twice

    bool isReadyToLaunch() const;    //! returns 'true' if all of this task's dependencies have been executed and the task is ready to launch

    /*! adds a task that depends on this task, i.e. provided task can only begin execution when this task is completed.
     Returns 'true' if specified dependency has been added successfully; returns 'false' if this dependency has already been
     declared for the node
    */
    bool addDependent(TaskGraphNode& task);    


protected:
    void forceUndone();    //! forces the task to appear as incompleted

private:
    uint32_t m_id;    //!< identifier of the node
    AbstractTask* m_contained_task;    //!< task contained by the node
    std::atomic_bool m_is_completed;    //!< equals 'true' if the task was completed. Equals 'false' otherwise
    bool m_is_scheduled;    //!< equals true if the node has already been scheduled, equals 'false' otherwise
    uint32_t m_visit_flag;    //!< determines how many time the node has been visited during task graph traversal (0:not visited; 1:visited once, 2:visited more than once)
    uint16_t m_frame_index;   //!< index of the frame, to which the task container belongs

    set_of_nodes_type m_dependencies;    //!< dependencies of this task. This task cannot run before all of its dependencies are executed
    set_of_nodes_type m_dependents;    //!< dependencies of this task. This task cannot be executed before the dependent tasks are completed
};

template<> class TaskGraphNodeAttorney<TaskGraph>
{
    friend class TaskGraph;

private:
    static inline unsigned char getNodeVisitFlag(TaskGraphNode const& parent_task_graph_node)
    {
        return parent_task_graph_node.m_visit_flag;
    }

    static inline void incrementNodeVisitFlag(TaskGraphNode& parent_task_graph_node)
    {
        ++parent_task_graph_node.m_visit_flag;
    }

    static inline void resetNodeVisitFlag(TaskGraphNode& parent_task_graph_node)
    {
        parent_task_graph_node.m_visit_flag = false;
    }

    static inline void setNodeFrameIndex(TaskGraphNode& parent_task_graph_node, uint16_t frame_index_value)
    {
        parent_task_graph_node.m_frame_index = frame_index_value;
    }

    static inline std::set<TaskGraphNode*> const& getDependentNodes(TaskGraphNode const& parent_task_graph_node)
    {
        return parent_task_graph_node.m_dependents;
    }

    static inline AbstractTask* getContainedTask(TaskGraphNode const& parent_task_graph_node)
    {
        return parent_task_graph_node.m_contained_task;
    }

    static inline uint32_t getNodeId(TaskGraphNode const& parent_task_graph_node)
    {
        return parent_task_graph_node.m_id;
    }

    static inline bool areNodesEqual(TaskGraphNode const& node1, TaskGraphNode const& node2)
    {
        return node1.m_id == node2.m_id;
    }

    static inline void resetNodeCompletionStatus(TaskGraphNode& parent_task_graph_node)
    {
        parent_task_graph_node.m_is_completed = false;
        parent_task_graph_node.m_is_scheduled = false;
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

}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH_NODE_H
#endif
