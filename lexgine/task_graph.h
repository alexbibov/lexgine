#ifndef LEXGINE_CORE_CONCURRENCY_TASK_GRAPH

#include "task_graph_node.h"

namespace lexgine {namespace core {namespace concurrency {

class TaskGraph : public NamedEntity<class_names::TaskGraph>
{
public:
    TaskGraph(std::string const& name, std::list<TaskGraphNode*> const& root_tasks, uint8_t num_workers = 8U);
    TaskGraph(std::list<TaskGraphNode*> const& root_tasks, uint8_t num_workers = 8U);

    uint8_t getNumberOfWorkerThreads() const;    //! returns number of worker threads assigned to the task graph

    void createDotRepresentation(std::string const& destination_path) const;    //! creates representation of the task graph using DOT language and saves it to the given destination path

private:
    void parse(std::list<TaskGraphNode*> const& root_tasks);    //! helper function used to simplify structure of the graph
	void set_frame_index(uint16_t frame_index);    //! sets index of the frame corresponding to this task graph

    uint8_t m_num_workers;    //!< number of worker threads assigned to the task graph
    std::list<std::unique_ptr<TaskGraphNode>> m_task_list;    //!< list of graph nodes (without repetitions)
};


}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH
#endif
