#ifndef LEXGINE_CORE_CONCURRENCY_TASK_GRAPH_NODE_H

#include "entity.h"
#include "optional.h"

#include <list>

namespace lexgine {namespace core {namespace concurrency {

class TaskGraph;
class AbstractTask;

template<typename> class TaskGraphNodeAttorney;

//! Implementation of task graph nodes
class TaskGraphNode
{
    friend class TaskGraphNodeAttorney<TaskGraph>;

public:
	TaskGraphNode(AbstractTask& task);

    bool execute(uint8_t worker_id);    //! executes task assigned to this node

    bool isCompleted() const;    //! returns 'true' if the task has been successfully completed. Returns 'false' otherwise
    
    bool isReadyToLaunch() const;    //! returns 'true' if all of this task's dependencies have been executed and the task is ready to launch

    void addDependent(TaskGraphNode& task);    //! adds task that depends on this task, i.e. provided task can only begin execution when this task is completed

private:
	AbstractTask& m_contained_task;    //!< task contained by the node
    bool m_is_completed;    //!< equals 'true' if the task was completed. Equals 'false' otherwise
	bool m_visit_flag;    //!< determines, whether the task node has been visited during task graph traversal
	uint16_t m_frame_index;   //!< index of the frame, to which the task container belongs

	std::list<TaskGraphNode*> m_dependencies;    //!< dependencies of this task. This task cannot run before all of its dependencies are executed
	std::list<TaskGraphNode*> m_dependents;    //!< dependencies of this task. This task cannot be executed before the dependent tasks are completed
};

template<> class TaskGraphNodeAttorney<TaskGraph>
{
public:
    static inline bool isNodeVisited(TaskGraphNode const& parent_task_graph_node)
    {
        return parent_task_graph_node.m_visit_flag;
    }

    static inline void setNodeVisitFlag(TaskGraphNode& parent_task_graph_node, bool visit_flag_value)
    {
        parent_task_graph_node.m_visit_flag = visit_flag_value;
    }

    static inline void setNodeFrameIndex(TaskGraphNode& parent_task_graph_node, uint16_t frame_index_value)
    {
        parent_task_graph_node.m_frame_index = frame_index_value;
    }
};

}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH_NODE_H
#endif
