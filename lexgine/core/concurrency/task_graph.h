#ifndef LEXGINE_CORE_CONCURRENCY_TASK_GRAPH

#include "task_graph_node.h"
#include "../class_names.h"

#include <iterator>

namespace lexgine {namespace core {namespace concurrency {


template<typename> class TaskGraphAttorney;
class TaskSink;

class TaskGraph : public NamedEntity<class_names::TaskGraph>
{
    friend class TaskGraphAttorney<TaskSink>;

public:
    class iterator : public std::iterator<std::bidirectional_iterator_tag, TaskGraphNode*, ptrdiff_t, TaskGraphNode*>
    {
        friend class TaskGraph;
    public:
        iterator();
        iterator(iterator const& other);
        iterator& operator=(iterator const& other);

        TaskGraphNode* operator*();
        TaskGraphNode const* operator*() const;
        TaskGraphNode* operator->();
        TaskGraphNode const*  operator->() const;

        iterator& operator++();
        iterator operator++(int);
        iterator& operator--();
        iterator operator--(int);

        bool operator==(iterator const& other) const;
        bool operator!=(iterator const& other) const;

    private:
        std::list<std::unique_ptr<TaskGraphNode>>::iterator m_backend_iterator;
    };
    using const_iterator = iterator const;
    
    TaskGraph(uint8_t num_workers = 8U, std::string const& name = "");
    TaskGraph(TaskGraphNode::set_of_nodes_type const& root_tasks, uint8_t num_workers = 8U, std::string const& name = "");
    TaskGraph(TaskGraph const& other) = delete;
    TaskGraph(TaskGraph&& other) = default;
    ~TaskGraph() = default;
    TaskGraph& operator=(TaskGraph const& other) = delete;
    TaskGraph& operator=(TaskGraph&& other) = default;

    void setRootNodes(TaskGraphNode::set_of_nodes_type const& root_tasks);    //! re-defines the task graph given a list of root nodes

    uint8_t getNumberOfWorkerThreads() const;    //! returns number of worker threads assigned to the task graph

    void createDotRepresentation(std::string const& destination_path) const;    //! creates representation of the task graph using DOT language and saves it to the given destination path

    void injectDependentTask(TaskGraphNode& dependent_task);    //! injects new task into the task graph, which will be dependent on all tasks currently in the task graph

    iterator begin();    //! returns iterator pointing at the first task of the task graph
    iterator end();    //! returns iterator pointing one step forward from the last task of the task graph

    const_iterator begin() const;    //! returns immutable iterator pointing at the first task of the task graph
    const_iterator end() const;    //! returns immutable iterator pointing one step forward from the last task of the task graph

private:
    void parse();    //! parses the task graph and creates local copy of its structure using provided set of root nodes as entry points
    void set_frame_index(uint16_t frame_index);    //! sets index of the frame corresponding to this task graph and ensures that all tasks are reseted to "incomplete" state
    void reset_completion_status();    //! resets completion status of all the nodes of the task graph

    uint8_t m_num_workers;    //!< number of worker threads assigned to the task graph
    TaskGraphNode::set_of_nodes_type m_root_nodes;    //!< set of pointers to nodes used to construct the task graph
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

    static inline void resetTaskGraphCompletionStatus(TaskGraph& parent_task_graph)
    {
        parent_task_graph.reset_completion_status();
    }

    static inline TaskGraphNode::set_of_nodes_type const& getTaskGraphRootNodeList(TaskGraph const& parent_task_graph)
    {
        return parent_task_graph.m_root_nodes;
    }
};


}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH
#endif
