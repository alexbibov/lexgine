#ifndef LEXGINE_CORE_CONCURRENCY_TASK_GRAPH
#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH

#include "task_graph_node.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/concurrency/lexgine_core_concurrency_fwd.h"

#include <iterator>

namespace lexgine::core::concurrency {

template<typename> class TaskGraphAttorney;

class TaskGraph : public NamedEntity<class_names::TaskGraph>
{
    friend class TaskGraphAttorney<TaskSink>;

public:
    using iterator = std::list<TaskGraphNode>::iterator;
    using const_iterator = std::list<TaskGraphNode>::const_iterator;

public:
    TaskGraph(uint8_t num_workers = 8U, std::string const& name = "");
    TaskGraph(std::set<TaskGraphRootNode const*> const& root_nodes,
        uint8_t num_workers = 8U, std::string const& name = "");
    TaskGraph(TaskGraph const& other) = delete;
    TaskGraph(TaskGraph&& other);
    ~TaskGraph() = default;
    TaskGraph& operator=(TaskGraph const& other) = delete;
    TaskGraph& operator=(TaskGraph&& other);

    void setRootNodes(std::set<TaskGraphRootNode const*> const& root_nodes);    //! re-defines the task graph given a list of root nodes

    uint8_t getNumberOfWorkerThreads() const;    //! returns number of worker threads assigned to the task graph

    void createDotRepresentation(std::string const& destination_path);    //! creates representation of the task graph using DOT language and saves it to the given destination path

    uint16_t frameIndex() const;    //! returns frame index, to which this task graph is assigned

    std::set<TaskGraphRootNode const*> const& rootNodes() const;

    iterator begin();    //! returns iterator pointing at the first task of the task graph
    iterator end();    //! returns iterator pointing one step forward from the last task of the task graph

    const_iterator begin() const;    //! returns immutable iterator pointing at the first task of the task graph
    const_iterator end() const;    //! returns immutable iterator pointing one step forward from the last task of the task graph

private:
    void compile(std::set<TaskGraphRootNode const*> const& root_nodes);    //! creates list of task graph nodes sorted in topological order, creates a full deep copy of the task graph as side effect
    void reset_completion_status();    //! resets completion status of all the nodes in the task graph
    
    /*! injects new task graph node that will be dependent on all nodes present in the task graph.
     This function is to be used only with already compiled task graph. Also, all dependencies present
     in the injected node at the moment of injection will be ignored.
    */
    void inject_dependent_node(TaskGraphNode const& node);    

private:
    uint8_t m_num_workers;    //!< number of worker threads assigned to the task graph
    std::set<TaskGraphRootNode const*> m_root_nodes;    //!< set of pointers to task graph root nodes
    std::list<TaskGraphNode> m_all_nodes;    //!< list of all graph nodes sorted in topological order
    uint16_t m_frame_index;    //!< index of the frame, to which this task graph corresponds
};


template<> class TaskGraphAttorney<TaskSink>
{
    friend class TaskSink;

private:
    static inline void resetTaskGraphCompletionStatus(TaskGraph& parent_task_graph)
    {
        parent_task_graph.reset_completion_status();
    }

    static inline TaskGraph cloneTaskGraphForFrame(TaskGraph const& parent_task_graph, uint16_t frame_index)
    {
        TaskGraph rv{ parent_task_graph.m_num_workers, parent_task_graph.getStringName() + "_frame_idx_copy#" + std::to_string(frame_index) };
        rv.m_frame_index = frame_index;
        rv.compile(parent_task_graph.m_root_nodes);
        return rv;
    } 

    static inline void injectDependentNode(TaskGraph& parent_task_graph, TaskGraphNode const& node)
    {
        parent_task_graph.inject_dependent_node(node);
    }
};


}

#endif    // LEXGINE_CORE_CONCURRENCY_TASK_GRAPH
