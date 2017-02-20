#include "task_graph.h"
#include "abstract_task.h"
#include <fstream>

using namespace lexgine::core::concurrency;


TaskGraph::TaskGraph(std::string const& name, std::list<ConcurrentTaskContainer> const& root_tasks, uint8_t num_workers):
    m_num_workers{ num_workers }
{
    setStringName(name);
    parse(root_tasks);
}

TaskGraph::TaskGraph(std::list<ConcurrentTaskContainer> const& root_tasks, uint8_t num_workers):
    m_num_workers{ num_workers }
{
    setStringName("DefaultTaskGraph" + getId().toString());
    parse(root_tasks);
}

uint8_t lexgine::core::concurrency::TaskGraph::getNumberOfWorkerThreads() const
{
    return m_num_workers;
}

void TaskGraph::createDotRepresentation(std::string const& destination_path) const
{
    std::string dot_graph_representation = "digraph " + getStringName() + " {\nnode[style=filled];\n";

    for (auto task : m_task_list)
    {
		AbstractTask* contained_task = ConcurrentTaskContainerAttorney<TaskGraph>::getContainedTaskPtr(task);

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

    for (auto task : m_task_list)
    {
        std::string task_string_id = "task" 
			+ ConcurrentTaskContainerAttorney<TaskGraph>::getContainedTaskPtr(task)->getId().toString();
        for (auto dependent_task : ConcurrentTaskContainerAttorney<TaskGraph>::getDependentTasks(task))
        {
            dot_graph_representation += task_string_id + "->" + "task" 
				+ ConcurrentTaskContainerAttorney<TaskGraph>::getContainedTaskPtr(dependent_task)->getId().toString() + ";\n";
        }
    }

    dot_graph_representation += "}\n";

    std::ofstream ofile{ destination_path.c_str() };
    if (ofile.bad())
        raiseError("Unable to write task graph representation to \"" + destination_path + "\"");
    ofile << dot_graph_representation;
    ofile.close();
}

void TaskGraph::parse(std::list<ConcurrentTaskContainer> const& root_tasks)
{
    using iterator_type = std::list<ConcurrentTaskContainer>::const_iterator;
    std::list<std::pair<iterator_type, iterator_type>> traversal_stack;
    traversal_stack.push_back(std::make_pair(root_tasks.begin(), root_tasks.end()));

    while (traversal_stack.size())
    {
        // sweep the graph downwards
        iterator_type current_task;
        while (ConcurrentTaskContainerAttorney<TaskGraph>::getDependentTasks(*(current_task = traversal_stack.back().first)).size())
        {
            if (!ConcurrentTaskContainerAttorney<TaskGraph>::visited(*current_task))
            {
                m_task_list.push_back(*current_task);
                ConcurrentTaskContainerAttorney<TaskGraph>::setVisitFlag(*current_task, true);
            }
            else
            {
                // the graph may contain cycles at this point

                bool cycle_presence_test = false;
                for (auto e = traversal_stack.begin(); e != --traversal_stack.end(); ++e)
                {
                    if (ConcurrentTaskContainerAttorney<TaskGraph>::getContainedTaskPtr(*e->first) 
						== ConcurrentTaskContainerAttorney<TaskGraph>::getContainedTaskPtr(*current_task))
                    {
                        cycle_presence_test = true;
                        break;
                    }
                }

                if (cycle_presence_test)
                {
                    std::string err_msg = "Task graph contains dependency cycles: " 
						+ ConcurrentTaskContainerAttorney<TaskGraph>::getContainedTaskPtr(*traversal_stack.begin()->first)->getStringName();
                    for (auto e = ++traversal_stack.begin(); e != traversal_stack.end(); ++e)
                    {
                        err_msg += "->" + ConcurrentTaskContainerAttorney<TaskGraph>::getContainedTaskPtr(*e->first)->getStringName();
                    }
                    err_msg += ConcurrentTaskContainerAttorney<TaskGraph>::getContainedTaskPtr(*current_task)->getStringName();
                    raiseError(err_msg);
                    break;
                }
            }


            auto current_task_list = ConcurrentTaskContainerAttorney<TaskGraph>::getDependentTasks(*current_task);
            traversal_stack.push_back(std::make_pair(current_task_list.begin(), current_task_list.end()));
        }

        if (getErrorState()) break;    // exit the main traversal loop in case if something went wrong during the down-sweep of the graph


        // parse the "leaf" task
        uint32_t current_task_visit_counter{  };
        if (!ConcurrentTaskContainerAttorney<TaskGraph>::visited(*current_task))
        {
            m_task_list.push_back(*current_task);
            ConcurrentTaskContainerAttorney<TaskGraph>::setVisitFlag(*current_task, true);
        }


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
		ConcurrentTaskContainerAttorney<TaskGraph>::setFrameIndex(task, frame_index);
	}
}
