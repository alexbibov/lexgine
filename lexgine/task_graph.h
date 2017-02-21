#ifndef LEXGINE_CORE_CONCURRENCY_TASK_GRAPH

#include "task_graph_node.h"
#include "class_names.h"

namespace lexgine {namespace core {namespace concurrency {


template<typename> class TaskGraphAttorney;
class TaskSink;

class TaskGraph : public NamedEntity<class_names::TaskGraph>
{
    friend class TaskGraphAttorney<TaskSink>;

public:
    TaskGraph(uint8_t num_workers = 8U, std::string const& name = "");
    TaskGraph(std::list<TaskGraphNode*> const& root_tasks, uint8_t num_workers = 8U, std::string const& name = "");
    TaskGraph(TaskGraph const& other) = delete;
    TaskGraph(TaskGraph&& other) = default;
    ~TaskGraph() = default;
    TaskGraph& operator=(TaskGraph const& other) = delete;
    TaskGraph& operator=(TaskGraph&& other) = default;

    void setRootNodes(std::list<TaskGraphNode*> const& root_tasks);    //! re-defines the task graph given a list of root nodes

    uint8_t getNumberOfWorkerThreads() const;    //! returns number of worker threads assigned to the task graph

    void createDotRepresentation(std::string const& destination_path) const;    //! creates representation of the task graph using DOT language and saves it to the given destination path

private:
    void parse(std::list<TaskGraphNode*> const& root_tasks);    //! helper function used to simplify structure of the graph
    void set_frame_index(uint16_t frame_index);    //! sets index of the frame corresponding to this task graph

    uint8_t m_num_workers;    //!< number of worker threads assigned to the task graph
    std::list<std::unique_ptr<TaskGraphNode>> m_task_list;    //!< list of graph nodes (without repetitions)
};


template<> class TaskGraphAttorney<TaskSink>
{
    friend class TaskSink;

private:
    static inline void setTaskGraphFrameIndex(TaskGraph& parent_task_graph, uint16_t frame_index)
    {
        parent_task_graph.set_frame_index(frame_index);
    }
};


}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH
#endif
