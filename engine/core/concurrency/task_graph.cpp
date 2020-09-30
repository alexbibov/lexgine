#include <unordered_set>
#include <deque>
#include <algorithm>

#include "task_graph.h"
#include "abstract_task.h"
#include "engine/core/exception.h"
#include <fstream>
#include <cassert>


using namespace lexgine::core::concurrency;

class TaskGraph::BarrierSyncTask : public AbstractTask
{
public:
    BarrierSyncTask() : AbstractTask{ "", false } {}

public:    // AbstractTask interface implementation
    bool doTask(uint8_t worker_id, uint64_t user_data) override { return true; }
    TaskType type() const override { return TaskType::cpu; }
};


TaskGraph::TaskGraph(uint8_t num_workers, std::string const& name) :
    m_num_workers{ num_workers },
    m_barrier_sync_task{ new BarrierSyncTask{} }
{
    setStringName(name);
}

TaskGraph::TaskGraph(std::set<TaskGraphRootNode const*> const& root_nodes,
    uint8_t num_workers, std::string const& name) :
    TaskGraph{ num_workers, name }
{
    setRootNodes(root_nodes);
}

TaskGraph::TaskGraph(TaskGraph&& other) :
    NamedEntity<class_names::TaskGraph>{ std::move(other) },
    m_num_workers{ other.m_num_workers },
    m_root_nodes{ std::move(other.m_root_nodes) },
    m_compiled_task_graph{ std::move(other.m_compiled_task_graph) },
    m_barrier_sync_task{ std::move(other.m_barrier_sync_task) }
{
}

TaskGraph::~TaskGraph() = default;

TaskGraph& TaskGraph::operator=(TaskGraph&& other)
{
    if (this == &other) return *this;

    NamedEntity<class_names::TaskGraph>::operator=(std::move(other));

    m_num_workers = other.m_num_workers;
    m_root_nodes = std::move(other.m_root_nodes);
    m_compiled_task_graph = std::move(other.m_compiled_task_graph);
    m_barrier_sync_task = std::move(other.m_barrier_sync_task);

    return *this;
}

void TaskGraph::setRootNodes(std::set<TaskGraphRootNode const*> const& root_nodes)
{
    m_root_nodes = root_nodes;
    m_compiled_task_graph.clear();
}

uint8_t lexgine::core::concurrency::TaskGraph::getNumberOfWorkerThreads() const
{
    return m_num_workers;
}

void TaskGraph::createDotRepresentation(std::string const& destination_path)
{
    compile();
    std::string dot_graph_representation = "digraph " + getStringName() + " {\nnode[style=filled];\n";

    for (auto& t : m_compiled_task_graph)
    {
        AbstractTask* contained_task = t.task();
        if (!AbstractTaskAttorney<TaskGraph>::isTaskExposedInDebugInformation(*contained_task)) continue;

        dot_graph_representation += "task" + contained_task->getId().toString() + "[label=\"" + contained_task->getStringName() + "\", ";
        switch (contained_task->type())
        {
        case TaskType::cpu:
            dot_graph_representation += "fillcolor=lightblue, shape=box";
            break;

        case TaskType::gpu_draw:
            dot_graph_representation += "fillcolor=yellow, shape=oval";
            break;

        case TaskType::gpu_compute:
            dot_graph_representation += "fillcolor=red, fontcolor=white, shape=hexagon";
            break;

        case TaskType::gpu_copy:
            dot_graph_representation += "fillcolor=gray, fontcolor=white, shape=diamond";
            break;

        case TaskType::other:
            dot_graph_representation += "fillcolor=white, shape=triangle";
            break;
        }
        dot_graph_representation += "];\n";
    }

    for (auto& t : m_compiled_task_graph)
    {
        AbstractTask* dependency_task = t.task();

        if (!AbstractTaskAttorney<TaskGraph>::isTaskExposedInDebugInformation(*dependency_task)) continue;

        std::string task_string_id = "task" + dependency_task->getId().toString();
        for (auto dependent : t.getDependents())
        {

            AbstractTask* dependent_task = dependent->task();
            if (!AbstractTaskAttorney<TaskGraph>::isTaskExposedInDebugInformation(*dependent_task)) continue;

            dot_graph_representation += task_string_id + "->" + "task"
                + dependent_task->getId().toString() + ";\n";
        }
    }

    dot_graph_representation += "}\n";

    std::ofstream ofile{ destination_path.c_str() };
    if (ofile.bad())
    {
        LEXGINE_LOG_ERROR(this, "Unable to write task graph representation to \"" + destination_path + "\"");
    }
    else
    {
        ofile << dot_graph_representation;
        ofile.close();
    }
}

std::set<TaskGraphRootNode const*> const& TaskGraph::rootNodes() const
{
    return m_root_nodes;
}

void TaskGraph::resetExecutionStatus()
{
    for (auto& n : m_compiled_task_graph)
    {
        n.resetExecutionStatus();
    }
}

void TaskGraph::setUserData(uint64_t user_data)
{
    for (auto& node : m_compiled_task_graph)
        node.setUserData(user_data);
}

void TaskGraph::compile()
{
    m_compiled_task_graph.clear();

    // perform DFS in order to create topological order for the graph
    {
        std::list<TaskGraphNode> dfs_stack;
        std::unordered_set<uint64_t> node_visit_temp_tlb, node_visit_perm_tlb;

        auto visit = [this, &dfs_stack, &node_visit_temp_tlb, &node_visit_perm_tlb](TaskGraphNode const& n)->void
        {
            auto visit_internal = [this, &dfs_stack, &node_visit_temp_tlb, &node_visit_perm_tlb](TaskGraphNode const& n, auto& myself)->void
            {
                if (node_visit_perm_tlb.find(n.getId()) != node_visit_perm_tlb.end()) return;

                auto [p, insertion_result] = node_visit_temp_tlb.insert(n.getId());

                if (!insertion_result)
                {
                    LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Unable to compile task graph: the graph contains cycles");
                }
                else
                {
                    for (auto e : n.getDependents())
                    {
                        myself(*e, myself);
                    }
                    dfs_stack.push_front(n.clone());
                    node_visit_perm_tlb.insert(*p);
                    node_visit_temp_tlb.erase(p);
                }
            };

            visit_internal(n, visit_internal);
        };


        for (auto e : m_root_nodes)
        {
            visit(*e);
        }

        m_compiled_task_graph = std::move(dfs_stack);
    }


    // copy dependencies
    {
        std::unordered_map<uint64_t, std::list<TaskGraphNode>::iterator> node_id_lut;

        for (auto p = m_compiled_task_graph.begin(); p != m_compiled_task_graph.end(); ++p)
        {
            node_id_lut.insert(std::make_pair(p->getId(), p));
        }


        // BFS. Well, this is not the real BFS, since we allow repeated traversal over the 
        // graph nodes in order to copy all of the dependencies. We do not care about possible
        // cycles as those would have been captured by the DFS above
        std::deque<TaskGraphNode const*> queue{};
        queue.insert(queue.end(), m_root_nodes.begin(), m_root_nodes.end());

        while (queue.size())
        {
            TaskGraphNode const* p_n = queue.front(); queue.pop_front();
            auto children = p_n->getDependents();

            auto parent_node_copy_iter = node_id_lut[p_n->getId()];
            for (auto n : children)
            {
                auto& child_node_copy_ref = *node_id_lut[n->getId()];
                parent_node_copy_iter->addDependent(child_node_copy_ref);
                queue.push_back(n);
            }
        }
    }

    // insert barrier synchronization
    {
        m_barrier_sync_task->setStringName(getStringName() + "__barrier_sync_task");
        m_compiled_task_graph.emplace_back(*m_barrier_sync_task);

        auto sync_node_iter = (++m_compiled_task_graph.rbegin()).base();
        for (auto p = m_compiled_task_graph.begin(); p != sync_node_iter; ++p) p->addDependent(*sync_node_iter);
    }
}

bool TaskGraph::isCompleted() const
{
    if (m_barrier_sync_task) return m_compiled_task_graph.back().isCompleted();

    return false;
}

TaskGraph::iterator TaskGraph::begin() { return m_compiled_task_graph.begin(); }

TaskGraph::iterator TaskGraph::end() { return m_compiled_task_graph.end(); }

TaskGraph::const_iterator TaskGraph::cbegin() const { return m_compiled_task_graph.cbegin(); }

TaskGraph::const_iterator TaskGraph::cend() const { return m_compiled_task_graph.cend(); }

TaskGraph::const_iterator TaskGraph::begin() const { return cbegin(); }

TaskGraph::const_iterator TaskGraph::end() const { return cend(); }

std::reverse_iterator<TaskGraph::iterator> TaskGraph::rbegin() { return m_compiled_task_graph.rbegin(); }

std::reverse_iterator<TaskGraph::iterator> TaskGraph::rend() { return m_compiled_task_graph.rend(); }

std::reverse_iterator<TaskGraph::const_iterator> TaskGraph::rbegin() const { return m_compiled_task_graph.rbegin(); }

std::reverse_iterator<TaskGraph::const_iterator> TaskGraph::rend() const { return m_compiled_task_graph.rend(); }
