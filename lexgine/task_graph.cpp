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
	std::string dot_graph_representation = "graph " + getStringName() + " {\n";

	for (auto task : m_task_list)
	{
		dot_graph_representation += "task" + task->getId().toString() + "[label=\"" + task->getStringName() + "\"];\n";
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

	auto read_current_task = [this, &traversal_stack]
	{
		// Read current task
		AbstractTask& traversed_task = **(traversal_stack.back().first);
		if (!AbstractTaskAttorney<TaskGraph>::visited(traversed_task))
		{
			m_task_list.push_back(&traversed_task);
			AbstractTaskAttorney<TaskGraph>::setVisitFlag(traversed_task, true);
		}
		else
		{
			// check presence of dependency cycle
			bool cycle_presence_test = true;
			for (auto e = traversal_stack.begin(); e != --traversal_stack.end(); ++e)
			{
				if (!AbstractTaskAttorney<TaskGraph>::visited(**(e->first)))
				{
					cycle_presence_test = false;
					break;
				}
			}

			if (cycle_presence_test)
				raiseError("Task graph contains dependency cycles");
		}
	};


	while (traversal_stack.size())
	{
		// sweep the graph downwards
		while (AbstractTaskAttorney<TaskGraph>::getDependentTasks(**(traversal_stack.back().first)).size())
		{
			std::list<AbstractTask*> const& current_task_list = AbstractTaskAttorney<TaskGraph>::getDependentTasks(**(traversal_stack.back().first));
			traversal_stack.push_back(std::make_pair(current_task_list.begin(), current_task_list.end()));
		}

		read_current_task();

		// sweep the graph upwards
		// if the current level of the graph is fully traversed, move one level up, read the preceding task and move to the next node in this upper level
		while (traversal_stack.size() && ++(traversal_stack.back().first) == traversal_stack.back().second)
		{
			traversal_stack.pop_back();
			if(traversal_stack.size()) read_current_task();
		}
	}
}
