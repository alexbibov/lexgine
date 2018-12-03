#include "task_graph.h"
#include "abstract_task.h"
#include "lexgine/core/exception.h"
#include <fstream>
#include <cassert>

#include <unordered_set>
#include <deque>


using namespace lexgine::core::concurrency;


TaskGraph::TaskGraph(uint8_t num_workers, std::string const& name):
    m_num_workers{ num_workers },
    m_frame_index{ 0U }
{
    setStringName(name);
}

TaskGraph::TaskGraph(std::set<TaskGraphRootNode const*> const& root_nodes, 
    uint8_t num_workers, std::string const& name):
    TaskGraph{ num_workers, name }
{
    setRootNodes(root_nodes);
}

TaskGraph::TaskGraph(TaskGraph&& other):
    NamedEntity<class_names::TaskGraph>{ std::move(other) },
    m_num_workers{ other.m_num_workers },
    m_root_nodes{ std::move(other.m_root_nodes) },
    m_all_nodes{ std::move(other.m_all_nodes) },
    m_frame_index{ other.m_frame_index }
{
}

TaskGraph& TaskGraph::operator=(TaskGraph&& other)
{
    if (this == &other) return *this;

    NamedEntity<class_names::TaskGraph>::operator=(std::move(other));

    m_num_workers = other.m_num_workers;
    m_root_nodes = std::move(other.m_root_nodes);
    m_all_nodes = std::move(other.m_all_nodes);
    m_frame_index = other.m_frame_index;

    return *this;
}

void TaskGraph::setRootNodes(std::set<TaskGraphRootNode const*> const& root_nodes)
{
    m_root_nodes = root_nodes;
}

uint8_t lexgine::core::concurrency::TaskGraph::getNumberOfWorkerThreads() const
{
    return m_num_workers;
}

void TaskGraph::createDotRepresentation(std::string const& destination_path)
{
    compile(m_root_nodes);
    std::string dot_graph_representation = "digraph " + getStringName() + " {\nnode[style=filled];\n";

    for (auto& task : m_all_nodes)
    {
        AbstractTask* contained_task = TaskGraphNodeAttorney<TaskGraph>::getContainedTask(task);
        if (!AbstractTaskAttorney<TaskGraph>::isExposedInTaskGraph(*contained_task)) continue;

        dot_graph_representation += "task" + contained_task->getId().toString() + "[label=\"" + contained_task->getStringName() + "\", ";
        switch (AbstractTaskAttorney<TaskGraph>::getTaskType(*contained_task))
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

    for (auto& task : m_all_nodes)
    {
        std::string task_string_id = "task"
            + TaskGraphNodeAttorney<TaskGraph>::getContainedTask(task)->getId().toString();
        for (auto dependent_task : TaskGraphNodeAttorney<TaskGraph>::getDependents(task))
        {
            dot_graph_representation += task_string_id + "->" + "task"
                + TaskGraphNodeAttorney<TaskGraph>::getContainedTask(*dependent_task)->getId().toString() + ";\n";
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

uint16_t TaskGraph::frameIndex() const
{
    return m_frame_index;
}

std::set<TaskGraphRootNode const*> const& TaskGraph::rootNodes() const
{
    return m_root_nodes;
}

TaskGraph::iterator TaskGraph::begin()
{
    return m_all_nodes.begin();
}

TaskGraph::iterator TaskGraph::end()
{
    return m_all_nodes.end();
}

TaskGraph::const_iterator TaskGraph::begin() const
{
    return m_all_nodes.begin();
}

TaskGraph::const_iterator TaskGraph::end() const
{
    return m_all_nodes.end();
}

void TaskGraph::compile(std::set<TaskGraphRootNode const*> const& root_nodes)
{
    // perform DFS in order to create topological order for the graph
    {
        std::list<TaskGraphNode> dfs_stack;
        std::unordered_set<uint64_t> node_visit_temp_tlb, node_visit_perm_tlb;

        auto visit = [this, &dfs_stack, &node_visit_temp_tlb, &node_visit_perm_tlb](TaskGraphNode const& n)->void
        {
            auto visit_internal = [this, &dfs_stack, &node_visit_temp_tlb, &node_visit_perm_tlb](TaskGraphNode const& n, auto& myself)->void
            {
                if (node_visit_perm_tlb.find(TaskGraphNodeAttorney<TaskGraph>::getNodeId(n)) != node_visit_perm_tlb.end()) return;

                auto[p, insertion_result] = node_visit_temp_tlb.insert(TaskGraphNodeAttorney<TaskGraph>::getNodeId(n));

                if (!insertion_result)
                {
                    LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Unable to compile task graph: the graph contains cycles");
                }
                else
                {
                    for (auto e : TaskGraphNodeAttorney<TaskGraph>::getDependents(n))
                    {
                        myself(*e, myself);
                    }
                    dfs_stack.push_front(TaskGraphNodeAttorney<TaskGraph>::cloneNodeForFrame(n, m_frame_index));
                    node_visit_perm_tlb.insert(*p);
                    node_visit_temp_tlb.erase(p);
                }
            };

            visit_internal(n, visit_internal);
        };


        for (auto e : root_nodes)
        {
            visit(*e);
        }

        m_all_nodes = std::move(dfs_stack);
    }


    // copy dependencies
    {
        std::unordered_map<uint64_t, std::list<TaskGraphNode>::iterator> node_id_lut;

        for (auto p = m_all_nodes.begin(); p != m_all_nodes.end(); ++p)
        {
            node_id_lut.insert(std::make_pair(TaskGraphNodeAttorney<TaskGraph>::getNodeId(*p), p));
        }


        // BFS
        std::deque<TaskGraphNode const*> queue{};
        queue.insert(queue.end(), root_nodes.begin(), root_nodes.end());

        while (queue.size())
        {
            TaskGraphNode const* p_n = queue.front(); queue.pop_front();
            auto children = TaskGraphNodeAttorney<TaskGraph>::getDependents(*p_n);

            auto parent_node_copy_iter = node_id_lut[TaskGraphNodeAttorney<TaskGraph>::getNodeId(*p_n)];
            for (auto n : children)
            {
                auto& child_node_copy_ref = *node_id_lut[TaskGraphNodeAttorney<TaskGraph>::getNodeId(*n)];
                parent_node_copy_iter->addDependent(child_node_copy_ref);
            }

            queue.insert(queue.end(), children.begin(), children.end());
        }
        

        // acquire pointers for the root nodes
        std::set<TaskGraphRootNode const*> cloned_root_nodes{};
        for (auto e : root_nodes)
        {
            TaskGraphNode& root_node_clone_ref = *node_id_lut[TaskGraphNodeAttorney<TaskGraph>::getNodeId(*e)];
            cloned_root_nodes.insert(reinterpret_cast<TaskGraphRootNode*>(&root_node_clone_ref));
        }
        m_root_nodes = cloned_root_nodes;
    }
}

void TaskGraph::reset_completion_status()
{
    for (auto& task : m_all_nodes)
    {
        TaskGraphNodeAttorney<TaskGraph>::resetNodeCompletionStatus(task);
    }
}

void TaskGraph::inject_dependent_node(TaskGraphNode const& dependent_node)
{
    m_all_nodes.emplace_back(TaskGraphNodeAttorney<TaskGraph>::cloneNodeForFrame(dependent_node, m_frame_index));

    TaskGraphNode& p_new_dependent_task = m_all_nodes.back();
    for (auto p = m_all_nodes.begin(); p != --m_all_nodes.end(); ++p)
        p->addDependent(p_new_dependent_task);
}