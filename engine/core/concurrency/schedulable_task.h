#ifndef LEXGINE_CORE_CONCURRENCY_SCHEDULABLE_TASK_H
#define LEXGINE_CORE_CONCURRENCY_SCHEDULABLE_TASK_H


#include "abstract_task.h"
#include "task_graph_node.h"

#include <mutex>

namespace lexgine::core::concurrency {

/*! Convenience class, which allows to create task graph
 structure transparently avoiding explicit usage of TaskGraphNode type
*/
class SchedulableTask : public AbstractTask, public TaskGraphNode
{
public:
    SchedulableTask(std::string const& debug_name = "", bool expose_in_task_graph = true);

};


//! Similar to SchedulableTask, but is used to creat root nodes of the task graph
class RootSchedulableTask : public AbstractTask, public TaskGraphRootNode
{
public:
    RootSchedulableTask(std::string const& debug_name = "");
};

/*! Convenience class, which allows to define tasks that cannot be executed concurrently.
    More precisely, if execute(...) is called concurrently for the same instance of SchedulableTaskWithoutConcurrency,
    the calls will be made sequentially. This is convenient when the task is using shared resource and is performed repeatedly
    on multiple threads.
*/
class SchedulableTaskWithoutConcurrency : public SchedulableTask
{
public:
    SchedulableTaskWithoutConcurrency(std::string const& debug_name = "", bool expose_in_task_graph = true);

    bool execute(uint8_t worker_id, uint16_t frame_index);

private:
    std::mutex m_task_execution_mutex;
};


//! Returns 'true' if given task supports concurrent execution. Returns 'false' otherwise.
bool checkConcurrentExecutionAbility(AbstractTask const& task);


#define ROOT_NODE_CAST(node_ptr) (reinterpret_cast<lexgine::core::concurrency::TaskGraphRootNode*>(static_cast<lexgine::core::concurrency::TaskGraphNode*>(node_ptr)))


}

#endif    // LEXGINE_CORE_CONCURRENCY_SCHEDULABLE_TASK_H
