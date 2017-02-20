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
    m_num_workers{ num_workers }
{
    setStringName(name);
    parse(root_tasks);
}

void TaskGraph::setRootNodes(std::list<TaskGraphNode*> const& root_tasks)
{
    m_task_list.clear();
    parse(root_tasks);
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

        case TaskType::exit:
            dot_graph_representation += "fillcolor=navy, fontcolor=white, shape=doublecircle";
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

void TaskGraph::parse(std::list<TaskGraphNode*> const& root_tasks)
{
    using iterator_type = std::list<TaskGraphNode*>::const_iterator;
    std::list<std::pair<iterator_type, iterator_type>> traversal_stack;
    traversal_stack.push_back(std::make_pair(root_tasks.begin(), root_tasks.end()));

    // lambda that attempts to copy incidence relation between given nodes
    auto establish_incidence = [this](TaskGraphNode const* parent_node, TaskGraphNode const* child_node) -> void
    {
        if (parent_node && (TaskGraphNodeAttorney<TaskGraph>::getNodeVisitFlag(*child_node) == 1
            || TaskGraphNodeAttorney<TaskGraph>::getNodeVisitFlag(*parent_node) == 1))
        {
            auto parent_node_copy_iter = std::find_if(m_task_list.rbegin(), m_task_list.rend(),
                [parent_node](std::unique_ptr<TaskGraphNode> const& node) -> bool
            {
                return TaskGraphNodeAttorney<TaskGraph>::getNodeId(*node) == TaskGraphNodeAttorney<TaskGraph>::getNodeId(*parent_node);
            });

            auto current_node_copy_iter = std::find_if(m_task_list.rbegin(), m_task_list.rend(),
                [child_node](std::unique_ptr<TaskGraphNode> const& node) -> bool
            {
                return TaskGraphNodeAttorney<TaskGraph>::getNodeId(*node) == TaskGraphNodeAttorney<TaskGraph>::getNodeId(*child_node);
            });

            (*parent_node_copy_iter)->addDependent(**current_node_copy_iter);
        }
    };


    while (traversal_stack.size())
    {
        // sweep the graph downwards
        TaskGraphNode* parent_node = nullptr;
        TaskGraphNode* current_node = nullptr;

        while (TaskGraphNodeAttorney<TaskGraph>::getDependentNodes(*(current_node = *traversal_stack.back().first)).size())
        {
            if (!TaskGraphNodeAttorney<TaskGraph>::getNodeVisitFlag(*current_node))
            {
                m_task_list.push_back(std::unique_ptr<TaskGraphNode>{
                    new TaskGraphNode{ *TaskGraphNodeAttorney<TaskGraph>::getContainedTask(*current_node), TaskGraphNodeAttorney<TaskGraph>::getNodeId(*current_node) }});
            }
            else
            {
                // the graph may contain cycles at this point

                bool cycle_presence_test = false;
                for (auto e = traversal_stack.begin(); e != --traversal_stack.end(); ++e)
                {
                    if (TaskGraphNodeAttorney<TaskGraph>::getNodeId(**e->first)
                        == TaskGraphNodeAttorney<TaskGraph>::getNodeId(*current_node))
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
            establish_incidence(parent_node, current_node);

            auto& current_task_list = TaskGraphNodeAttorney<TaskGraph>::getDependentNodes(*current_node);
            traversal_stack.push_back(std::make_pair(current_task_list.begin(), current_task_list.end()));

            parent_node = current_node;
        }

        if (getErrorState()) break;    // exit the main traversal loop in case if something went wrong during the down-sweep of the graph


        // parse the "leaf" task
        if (!TaskGraphNodeAttorney<TaskGraph>::getNodeVisitFlag(*current_node))
        {
            m_task_list.push_back(std::unique_ptr<TaskGraphNode>{
                new TaskGraphNode{*TaskGraphNodeAttorney<TaskGraph>::getContainedTask(*current_node), TaskGraphNodeAttorney<TaskGraph>::getNodeId(*current_node) } });
        }
        TaskGraphNodeAttorney<TaskGraph>::incrementNodeVisitFlag(*current_node);
        
        // copy incidence parameters of the leaf
        establish_incidence(parent_node, current_node);


        // sweep the graph upwards
        // if the current level of the graph is fully traversed, move one level up
        while (traversal_stack.size() && ++(traversal_stack.back().first) == traversal_stack.back().second)
        {
            traversal_stack.pop_back();
        }
    }
}

void TaskGraph::set_frame_index(uint16_t frame_index)
{
    for (auto& task : m_task_list)
    {
        TaskGraphNodeAttorney<TaskGraph>::setNodeFrameIndex(*task, frame_index);
    }
}
