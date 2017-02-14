#include "task_graph.h"
#include <fstream>

using namespace lexgine::core::concurrency;


TaskGraph::TaskGraph(std::string const& name, std::list<AbstractTask*> const& root_tasks, uint8_t num_workers):
    m_num_workers{ num_workers }
{
    setStringName(name);
    parse(root_tasks);
}

TaskGraph::TaskGraph(std::list<AbstractTask*> const& root_tasks, uint8_t num_workers):
    m_num_workers{ num_workers }
{
    setStringName("DefaultTaskGraph" + getId().toString());
    parse(root_tasks);
}

uint8_t lexgine::core::concurrency::TaskGraph::getNumberOfWorkerThreads() const
{
    return m_num_workers;
}

void lexgine::core::concurrency::TaskGraph::execute()
{
}

void TaskGraph::createDotRepresentation(std::string const& destination_path) const
{
    std::string dot_graph_representation = "digraph " + getStringName() + " {\nnode[style=filled];\n";

    for (auto task : m_task_list)
    {
        dot_graph_representation += "task" + task->getId().toString() + "[label=\"" + task->getStringName() + "\", ";
        switch (AbstractTaskAttorney<TaskGraph>::getTaskType(*task))
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

    for (auto task : m_task_list)
    {
        std::string task_string_id = "task" + task->getId().toString();
        for (auto dependent_task : AbstractTaskAttorney<TaskGraph>::getDependentTasks(*task))
        {
            dot_graph_representation += task_string_id + "->" + "task" + dependent_task->getId().toString() + ";\n";
        }
    }

    dot_graph_representation += "}\n";

    std::ofstream ofile{ destination_path.c_str() };
    if (ofile.bad())
        raiseError("Unable to write task graph representation to \"" + destination_path + "\"");
    ofile << dot_graph_representation;
    ofile.close();
}

void TaskGraph::parse(std::list<AbstractTask*> const& root_tasks)
{
    using iterator_type = std::list<AbstractTask*>::const_iterator;
    std::list<std::pair<iterator_type, iterator_type>> traversal_stack;
    traversal_stack.push_back(std::make_pair(root_tasks.begin(), root_tasks.end()));

    while (traversal_stack.size())
    {
        // sweep the graph downwards
        AbstractTask* current_task;
        while (AbstractTaskAttorney<TaskGraph>::getDependentTasks(*(current_task = *traversal_stack.back().first)).size())
        {
            if (!AbstractTaskAttorney<TaskGraph>::visited(*current_task))
            {
                m_task_list.push_back(current_task);
                AbstractTaskAttorney<TaskGraph>::setVisitFlag(*current_task, true);
            }
            else
            {
                // the graph may contain cycles at this point

                bool cycle_presence_test = false;
                for (auto e = traversal_stack.begin(); e != --traversal_stack.end(); ++e)
                {
                    if (*e->first == current_task)
                    {
                        cycle_presence_test = true;
                        break;
                    }
                }

                if (cycle_presence_test)
                {
                    std::string err_msg = "Task graph contains dependency cycles: " + (*traversal_stack.begin()->first)->getStringName();
                    for (auto e = ++traversal_stack.begin(); e != traversal_stack.end(); ++e)
                    {
                        err_msg += "->" + (*e->first)->getStringName();
                    }
                    err_msg += current_task->getStringName();
                    raiseError(err_msg);
                    break;
                }
            }


            std::list<AbstractTask*> const& current_task_list = AbstractTaskAttorney<TaskGraph>::getDependentTasks(*current_task);
            traversal_stack.push_back(std::make_pair(current_task_list.begin(), current_task_list.end()));
        }

        if (getErrorState()) break;    // exit the main traversal loop in case if something went wrong during the down-sweep of the graph


        // parse the "leaf" task
        uint32_t current_task_visit_counter{  };
        if (!AbstractTaskAttorney<TaskGraph>::visited(*current_task))
        {
            m_task_list.push_back(current_task);
            AbstractTaskAttorney<TaskGraph>::setVisitFlag(*current_task, true);
        }


        // sweep the graph upwards
        // if the current level of the graph is fully traversed, move one level up
        while (traversal_stack.size() && ++(traversal_stack.back().first) == traversal_stack.back().second)
        {
            traversal_stack.pop_back();
        }
    }
}
