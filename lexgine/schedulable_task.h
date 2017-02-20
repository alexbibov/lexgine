#ifndef LEXGINE_CORE_CONCURRENCY_SCHEDULABLE_TASK_H

#include "abstract_task.h"
#include "task_graph_node.h"

namespace lexgine {namespace core {namespace concurrency {

/*! Convenience class, which allows to create task graph 
 structure transparently avoiding explicit usage of TaskGraphNode type
*/
class SchedulableTask : public AbstractTask, public TaskGraphNode
{
public:
    SchedulableTask(std::string const& debug_name = "");
};

}}}


#define LEXGINE_CORE_CONCURRENCY_SCHEDULABLE_TASK_H
#endif
