#include "rendering_tasks.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"

using namespace lexgine::core;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::dx::d3d12;

RenderingTasks::RenderingTasks(Globals const& globals, 
    std::set<concurrency::TaskGraphRootNode const*> const& entry_tasks):
    m_globals{ globals },
    m_task_graph{entry_tasks, globals.get<GlobalSettings>()->getNumberOfWorkers(), "RenderingTasksGraph" },
    m_task_sink{ m_task_graph, **globals.get<std::vector<std::ostream*>*>(), globals.get<GlobalSettings>()->getMaxFramesInFlight(), "RenderingTasksSink" }
{
}
