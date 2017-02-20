#ifndef LEXGINE_CORE_CONCURRENCY_TASK_GRAPH_NODE_H

#include "entity.h"
#include "optional.h"

#include <list>
#include <algorithm>

namespace lexgine {namespace core {namespace concurrency {

class TaskGraph;
class AbstractTask;

template<typename> class TaskGraphNodeAttorney;

//! Implementation of task graph nodes
class TaskGraphNode
{
    friend class TaskGraphNodeAttorney<TaskGraph>;

public:
    TaskGraphNode(AbstractTask& task);    //! creates task graph node encapsulating given task
    TaskGraphNode(AbstractTask&task, uint32_t id);    //! creates task graph node, which encapsulates the given task and has provided identifier
    TaskGraphNode(TaskGraphNode const& other) = delete;
    TaskGraphNode(TaskGraphNode&&) = default;

    TaskGraphNode& operator=(TaskGraphNode const&) = delete;
    TaskGraphNode& operator=(TaskGraphNode&&) = default;

    ~TaskGraphNode() = default;


    bool execute(uint8_t worker_id);    //! executes task assigned to this node

    bool isCompleted() const;    //! returns 'true' if the task has been successfully completed. Returns 'false' otherwise

    bool isReadyToLaunch() const;    //! returns 'true' if all of this task's dependencies have been executed and the task is ready to launch

    void addDependent(TaskGraphNode& task);    //! adds a task that depends on this task, i.e. provided task can only begin execution when this task is completed

private:
    uint32_t m_id;    //!< identifier of the node
    AbstractTask* m_contained_task;    //!< task contained by the node
    bool m_is_completed;    //!< equals 'true' if the task was completed. Equals 'false' otherwise
    unsigned char m_visit_flag;    //!< determines how many time the node has been visited during task graph traversal (0:not visited; 1:visited once, 2:visited more than once)
    uint16_t m_frame_index;   //!< index of the frame, to which the task container belongs

    std::list<TaskGraphNode*> m_dependencies;    //!< dependencies of this task. This task cannot run before all of its dependencies are executed
    std::list<TaskGraphNode*> m_dependents;    //!< dependencies of this task. This task cannot be executed before the dependent tasks are completed
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
        parent_task_graph_node.m_visit_flag = (std::min)(parent_task_graph_node.m_visit_flag + 1U, 2U);
    }

    static inline void setNodeFrameIndex(TaskGraphNode& parent_task_graph_node, uint16_t frame_index_value)
    {
        parent_task_graph_node.m_frame_index = frame_index_value;
    }

    static inline std::list<TaskGraphNode*> const& getDependentNodes(TaskGraphNode const& parent_task_graph_node)
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
};

}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH_NODE_H
#endif
