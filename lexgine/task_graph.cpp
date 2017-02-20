#include "task_graph.h"
#include "abstract_task.h"
#include <fstream>
#include <algorithm>
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
    std::list<TaskGraphNode*> incidence_reconstruction_stack;
    traversal_stack.push_back(std::make_pair(root_tasks.begin(), root_tasks.end()));

    while (traversal_stack.size())
    {
        // sweep the graph downwards
        TaskGraphNode* current_node = nullptr;
        while (TaskGraphNodeAttorney<TaskGraph>::getDependentNodes(*(current_node = *traversal_stack.back().first)).size())
        {
            if (!TaskGraphNodeAttorney<TaskGraph>::isNodeVisited(*current_node))
            {
                m_task_list.push_back(std::unique_ptr<TaskGraphNode>{
                    new TaskGraphNode{ *TaskGraphNodeAttorney<TaskGraph>::getContainedTask(*current_node), TaskGraphNodeAttorney<TaskGraph>::getNodeId(*current_node) }});
                incidence_reconstruction_stack.push_back(m_task_list.back().get());
                TaskGraphNodeAttorney<TaskGraph>::setNodeVisitFlag(*current_node, true);
            }
            else
            {
                // the graph may contain cycles at this point

                bool cycle_presence_test = false;
                for (auto e = traversal_stack.begin(); e != --traversal_stack.end(); ++e)
                {
                    if (TaskGraphNodeAttorney<TaskGraph>::getContainedTask(**e->first)
                        == TaskGraphNodeAttorney<TaskGraph>::getContainedTask(*current_node))
                    {
                        cycle_presence_test = true;
                        break;
                    }
                }

                if (cycle_presence_test)
                {
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


                uint32_t target_id = TaskGraphNodeAttorney<TaskGraph>::getNodeId(*current_node);
                auto target_node_iterator = std::find_if(m_task_list.begin(), m_task_list.end(),
                    [target_id](std::unique_ptr<TaskGraphNode> const& node) -> bool
                {
                    return TaskGraphNodeAttorney<TaskGraph>::getNodeId(*node) == target_id;
                });

                assert(target_node_iterator != m_task_list.end());

                incidence_reconstruction_stack.push_back(target_node_iterator->get());
            }


            auto& current_task_list = TaskGraphNodeAttorney<TaskGraph>::getDependentNodes(*current_node);
            traversal_stack.push_back(std::make_pair(current_task_list.begin(), current_task_list.end()));
        }

        if (getErrorState()) break;    // exit the main traversal loop in case if something went wrong during the down-sweep of the graph


        // parse the "leaf" task
        if (!TaskGraphNodeAttorney<TaskGraph>::isNodeVisited(*current_node))
        {
            m_task_list.push_back(std::unique_ptr<TaskGraphNode>{
                new TaskGraphNode{*TaskGraphNodeAttorney<TaskGraph>::getContainedTask(*current_node), TaskGraphNodeAttorney<TaskGraph>::getNodeId(*current_node) } });
            TaskGraphNodeAttorney<TaskGraph>::setNodeVisitFlag(*current_node, true);

            if (incidence_reconstruction_stack.size())
            {
                incidence_reconstruction_stack.back()->addDependent(*m_task_list.back());
            }
        }
        else if(incidence_reconstruction_stack.size())
        {
            // if the "leaf" node is visited more than once it must already reside in m_task_list

            uint32_t target_id = TaskGraphNodeAttorney<TaskGraph>::getNodeId(*current_node);
            auto target_node_iterator = std::find_if(m_task_list.begin(), m_task_list.end(),
                [target_id](std::unique_ptr<TaskGraphNode> const& node) -> bool
            {
                return TaskGraphNodeAttorney<TaskGraph>::getNodeId(*node) == target_id;
            });

            assert(target_node_iterator != m_task_list.end());

            incidence_reconstruction_stack.back()->addDependent(**target_node_iterator);
        }


        // sweep the graph upwards
        // if the current level of the graph is fully traversed, move one level up
        while (traversal_stack.size() && ++(traversal_stack.back().first) == traversal_stack.back().second)
        {
            traversal_stack.pop_back();
            if (incidence_reconstruction_stack.size())
            {
                TaskGraphNode* target_node = incidence_reconstruction_stack.back();
                incidence_reconstruction_stack.pop_back();
                if (incidence_reconstruction_stack.size()) incidence_reconstruction_stack.back()->addDependent(*target_node);
            }
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
