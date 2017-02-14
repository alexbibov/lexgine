#ifndef LEXGINE_CORE_CONCURRENCY_TASK_GRAPH

#include "task.h"

namespace lexgine {namespace core {namespace concurrency {

class TaskGraph : public NamedEntity<class_names::TaskGraph>
{
public:
	TaskGraph(std::string const& name, std::list<AbstractTask*> const& root_tasks, uint8_t num_workers = 8U);
    TaskGraph(std::list<AbstractTask*> const& root_tasks, uint8_t num_workers = 8U);

    uint8_t getNumberOfWorkerThreads() const;    //! returns number of worker threads assigned to the task graph

    void execute();    //! executes the task graph

    void createDotRepresentation(std::string const& destination_path) const;    //! creates representation of the task graph using DOT language and saves it to the given destination path

private:
	void parse(std::list<AbstractTask*> const& root_tasks);    //! helper function used to simplify structure of the graph

    uint8_t m_num_workers;    //!< number of worker threads assigned to the task graph
	std::list<AbstractTask*> m_task_list;    //!< list of graph nodes (without repetitions)
};

}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH
#endif
