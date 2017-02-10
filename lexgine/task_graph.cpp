#include "task_graph.h"

using namespace lexgine::core::concurrency;

TaskGraph::TaskGraph(std::list<AbstractTask*> const& root_tasks, uint8_t num_workers):
    m_root_tasks{ root_tasks },
    m_num_workers{ num_workers }
{
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
    std::string dot_graph_representation;

}
