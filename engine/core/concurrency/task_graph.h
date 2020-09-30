#ifndef LEXGINE_CORE_CONCURRENCY_TASK_GRAPH
#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH

#include "task_graph_node.h"
#include "engine/core/class_names.h"
#include "engine/core/concurrency/lexgine_core_concurrency_fwd.h"

#include <iterator>

namespace lexgine::core::concurrency {

template<typename T> class TaskGraphAttorney;

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
    ~TaskGraph();
    TaskGraph& operator=(TaskGraph const& other) = delete;
    TaskGraph& operator=(TaskGraph&& other);

    //! Sets new root nodes for the task graph
    void setRootNodes(std::set<TaskGraphRootNode const*> const& root_nodes);    

    uint8_t getNumberOfWorkerThreads() const;    //! returns number of worker threads assigned to the task graph

    void createDotRepresentation(std::string const& destination_path);    //! creates representation of the task graph using DOT language and saves it to the given destination path

    std::set<TaskGraphRootNode const*> const& rootNodes() const; 

    //! Resets execution status of the task graph (effectively, this resets execution status of every node of the graph)
    void resetExecutionStatus();

    //! Sets custom user data, which will be associated with every node in the task graph. The task graph must be compiled before calling this function.
    void setUserData(uint64_t user_data);

    // support of iteration over compiled graph nodes

    iterator begin();
    iterator end();

    const_iterator cbegin() const;
    const_iterator cend() const;
    const_iterator begin() const;
    const_iterator end() const;

    std::reverse_iterator<iterator> rbegin();
    std::reverse_iterator<iterator> rend();

    std::reverse_iterator<const_iterator> rbegin() const;
    std::reverse_iterator<const_iterator> rend() const;

private:
    class BarrierSyncTask;

private:
    /*! Prepares the task graph for execution. This process creates the internal list containing all the nodes of the task graph
     sorted using topological ordering. As a side effect this routine generates a deep copy of the task graph.
    */
    void compile();

    /*! For compiled task graph returns 'true' when its execution has been finished, otherwise returns 'false'.
     If the task graph was not compiled, the results of this function's invocation are undefined
    */
    bool isCompleted() const;

private:
    uint8_t m_num_workers;    //!< number of worker threads assigned to the task graph
    std::set<TaskGraphRootNode const*> m_root_nodes;    //!< set of pointers to task graph root nodes
    std::list<TaskGraphNode> m_compiled_task_graph;    //!< list of all graph nodes sorted in topological order
    std::unique_ptr<BarrierSyncTask> m_barrier_sync_task;    //!< barrier synchronization dummy task used to determine when execution of compiled task graph is finished
};

template<> class TaskGraphAttorney<TaskSink>
{
    friend TaskSink;

    static void compileTaskGraph(TaskGraph& source_task_graph)
    {
        source_task_graph.compile();
    }

    static bool isTaskGraphCompleted(TaskGraph const& compiled_task_graph)
    {
        return compiled_task_graph.isCompleted();
    }
};

}

#endif    // LEXGINE_CORE_CONCURRENCY_TASK_GRAPH
