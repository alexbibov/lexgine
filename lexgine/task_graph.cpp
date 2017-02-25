#include "task_graph.h"
#include "abstract_task.h"
#include <fstream>
#include <cassert>

using namespace lexgine::core::concurrency;


TaskGraph::TaskGraph(uint8_t num_workers, std::string const& name):
    m_num_workers{ num_workers }
{
    setStringName(name);
}

TaskGraph::TaskGraph(std::list<TaskGraphNode*> const& root_tasks, uint8_t num_workers, std::string const& name):
    m_root_nodes{ root_tasks },
    m_num_workers{ num_workers }
{
    setStringName(name);
    parse();
}

void TaskGraph::setRootNodes(std::list<TaskGraphNode*> const& root_tasks)
{
    m_task_list.clear();
    m_root_nodes = root_tasks;
    parse();
}

uint8_t lexgine::core::concurrency::TaskGraph::getNumberOfWorkerThreads() const
{
    return m_num_workers;
}

void TaskGraph::createDotRepresentation(std::string const& destination_path) const
{
    std::string dot_graph_representation = "digraph " + getStringName() + " {\nnode[style=filled];\n";

    for (auto& task : m_task_list)
    {
        AbstractTask* contained_task = TaskGraphNodeAttorney<TaskGraph>::getContainedTask(*task);
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

    for (auto& task : m_task_list)
    {
        std::string task_string_id = "task"
            + TaskGraphNodeAttorney<TaskGraph>::getContainedTask(*task)->getId().toString();
        for (auto dependent_task : TaskGraphNodeAttorney<TaskGraph>::getDependentNodes(*task))
        {
            dot_graph_representation += task_string_id + "->" + "task"
                + TaskGraphNodeAttorney<TaskGraph>::getContainedTask(*dependent_task)->getId().toString() + ";\n";
        }
    }

    dot_graph_representation += "}\n";

    std::ofstream ofile{ destination_path.c_str() };
    if (ofile.bad())
        raiseError("Unable to write task graph representation to \"" + destination_path + "\"");
    ofile << dot_graph_representation;
    ofile.close();
}

void TaskGraph::injectDependentTask(TaskGraphNode& dependent_task)
{
    m_task_list.push_back(std::unique_ptr<TaskGraphNode>{ new TaskGraphNode{
        *TaskGraphNodeAttorney<TaskGraph>::getContainedTask(dependent_task), TaskGraphNodeAttorney<TaskGraph>::getNodeId(dependent_task) }});

    TaskGraphNode* p_new_dependent_task = m_task_list.back().get();
    for (auto p = m_task_list.begin(); p != --m_task_list.end(); ++p)
        (*p)->addDependent(*p_new_dependent_task);
}

TaskGraph::iterator TaskGraph::begin()
{
    iterator begin_iter{};
    begin_iter.m_backend_iterator = m_task_list.begin();
    return begin_iter;
}

TaskGraph::iterator TaskGraph::end()
{
    iterator end_iter{};
    end_iter.m_backend_iterator = m_task_list.end();
    return end_iter;
}

TaskGraph::const_iterator TaskGraph::begin() const
{
    return const_cast<TaskGraph*>(this)->begin();
}

TaskGraph::const_iterator TaskGraph::end() const
{
    return const_cast<TaskGraph*>(this)->end();
}

void TaskGraph::parse()
{
    using iterator_type = std::list<TaskGraphNode*>::const_iterator;
    std::list<std::pair<iterator_type, iterator_type>> traversal_stack;
    traversal_stack.push_back(std::make_pair(m_root_nodes.begin(), m_root_nodes.end()));

    std::list<TaskGraphNode*> source_node_list{};
    TaskGraphNode* parent_node = nullptr;
    while (traversal_stack.size())
    {
        // sweep the graph downwards
        TaskGraphNode* current_node = *traversal_stack.back().first;

        if (!TaskGraphNodeAttorney<TaskGraph>::getNodeVisitFlag(*current_node))
        {
            m_task_list.push_back(std::unique_ptr<TaskGraphNode>{
                new TaskGraphNode{ *TaskGraphNodeAttorney<TaskGraph>::getContainedTask(*current_node), TaskGraphNodeAttorney<TaskGraph>::getNodeId(*current_node) }});
            source_node_list.push_back(current_node);
        }
        else
        {
            // the graph may contain cycles at this point
            bool cycle_presence_test = false;
            for (auto e = traversal_stack.begin(); e != --traversal_stack.end(); ++e)
            {
                if (TaskGraphNodeAttorney<TaskGraph>::areNodesEqual(**e->first, *current_node))
                {
                    cycle_presence_test = true;
                    break;
                }
            }

            if (cycle_presence_test)
            {
                // create error message describing where exactly the cycle occurs
                std::string err_msg = "Task graph contains dependency cycles: "
                    + TaskGraphNodeAttorney<TaskGraph>::getContainedTask(**traversal_stack.begin()->first)->getStringName();
                for (auto e = ++traversal_stack.begin(); e != traversal_stack.end(); ++e)
                {
                    err_msg += "->" + TaskGraphNodeAttorney<TaskGraph>::getContainedTask(**e->first)->getStringName();
                }
                err_msg += TaskGraphNodeAttorney<TaskGraph>::getContainedTask(*current_node)->getStringName();
                raiseError(err_msg);
                break;
            }
        }
        TaskGraphNodeAttorney<TaskGraph>::incrementNodeVisitFlag(*current_node);

        // copy incidence parameters of the graph
        if (parent_node && (TaskGraphNodeAttorney<TaskGraph>::getNodeVisitFlag(*current_node) == 1
            || TaskGraphNodeAttorney<TaskGraph>::getNodeVisitFlag(*parent_node) == 1))
        {
            auto parent_node_copy_iter = std::find_if(m_task_list.rbegin(), m_task_list.rend(),
                [parent_node](std::unique_ptr<TaskGraphNode> const& node) -> bool
            {
                return TaskGraphNodeAttorney<TaskGraph>::areNodesEqual(*node, *parent_node);
            });

            auto current_node_copy_iter = std::find_if(m_task_list.rbegin(), m_task_list.rend(),
                [current_node](std::unique_ptr<TaskGraphNode> const& node) -> bool
            {
                return TaskGraphNodeAttorney<TaskGraph>::areNodesEqual(*node, *current_node);
            });

            (*parent_node_copy_iter)->addDependent(**current_node_copy_iter);
        }

        auto& current_task_list = TaskGraphNodeAttorney<TaskGraph>::getDependentNodes(*current_node);
        if (current_task_list.size())
        {
            traversal_stack.push_back(std::make_pair(current_task_list.begin(), current_task_list.end()));
            parent_node = current_node;
        }
        else
        {
            // sweep the graph upwards
            // if the current level of the graph is fully traversed, move one level up
            while (traversal_stack.size() && ++(traversal_stack.back().first) == traversal_stack.back().second)
            {
                traversal_stack.pop_back();
            }

            if (traversal_stack.size() > 1)
            {
                parent_node = *(++traversal_stack.rbegin())->first;
            }
            else
            {
                parent_node = nullptr;
            }
        }
    }

    // reset visit flag of the source nodes
    for (auto p_node : source_node_list)
        TaskGraphNodeAttorney<TaskGraph>::resetNodeVisitFlag(*p_node);
}

void TaskGraph::set_frame_index(uint16_t frame_index)
{
    for (auto& task : m_task_list)
    {
        TaskGraphNodeAttorney<TaskGraph>::setNodeFrameIndex(*task, frame_index);
    }
}

void TaskGraph::reset_completion_status()
{
    for (auto& task : m_task_list)
    {
        TaskGraphNodeAttorney<TaskGraph>::resetNodeCompletionStatus(*task);
    }
}

TaskGraph::iterator::iterator()
{
}

TaskGraph::iterator::iterator(iterator const& other):
    m_backend_iterator{ other.m_backend_iterator }
{
}

TaskGraph::iterator& TaskGraph::iterator::operator=(iterator const& other)
{
    m_backend_iterator = other.m_backend_iterator;
    return *this;
}
 
TaskGraphNode* TaskGraph::iterator::operator*()
{
    return m_backend_iterator->get();
}

TaskGraphNode const* TaskGraph::iterator::operator*() const
{
    return m_backend_iterator->get();
}

TaskGraphNode* TaskGraph::iterator::operator->()
{
    return m_backend_iterator->get();
}

TaskGraphNode const* TaskGraph::iterator::operator->() const
{
    return m_backend_iterator->get();
}

TaskGraph::iterator& TaskGraph::iterator::operator++()
{
    ++m_backend_iterator;
    return *this;
}

TaskGraph::iterator TaskGraph::iterator::operator++(int)
{
    iterator tmp{ *this };
    ++m_backend_iterator;
    return tmp;
}

TaskGraph::iterator& TaskGraph::iterator::operator--()
{
    --m_backend_iterator;
    return *this;
}

TaskGraph::iterator TaskGraph::iterator::operator--(int)
{
    iterator tmp{ *this };
    --m_backend_iterator;
    return tmp;
}

bool TaskGraph::iterator::operator==(iterator const& other) const
{
    return m_backend_iterator == other.m_backend_iterator;
}

bool TaskGraph::iterator::operator!=(iterator const & other) const
{
    return !(*this == other);
}


