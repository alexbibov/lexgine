#ifndef LEXGINE_CORE_CONCURRENCY_TASK_GRAPH

#include "task.h"

namespace lexgine {namespace core {namespace concurrency {

class TaskGraph
{
public:
    TaskGraph(std::list<AbstractTask*> const& root_tasks, uint8_t num_workers = 8U);

    uint8_t getNumberOfWorkerThreads() const;    //! returns number of worker threads assigned to the task graph

    void execute();    //! executes the task graph

    void createDotRepresentation(std::string const& destination_path) const;    //! creates representation of the task graph using DOT language and saves it to the given destination path

private:
    std::list<AbstractTask*> m_root_tasks;    //!< list of the root tasks of the task graph
    uint8_t m_num_workers;    //!< number of worker threads assigned to the task graph
};

}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_GRAPH
#endif
