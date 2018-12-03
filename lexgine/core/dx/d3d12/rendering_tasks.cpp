#include "rendering_tasks.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"

using namespace lexgine::core;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::dx::d3d12;

RenderingTasks::RenderingTasks(Globals const& globals, 
    std::set<TaskGraphRootNode*> const& entry_tasks,
    TaskGraphNode* finalizing_task):
    m_globals{ globals },

    m_task_graph{ std::set<TaskGraphRootNode const*>{entry_tasks.begin(), entry_tasks.end()}, 
    globals.get<GlobalSettings>()->getNumberOfWorkers(), "RenderingTasksGraph" },

    m_task_sink{ m_task_graph, **globals.get<std::vector<std::ostream*>*>(), 
    globals.get<GlobalSettings>()->getMaxFramesInFlight(), "RenderingTasksSink" },

    m_finalizing_task_ptr{ finalizing_task }
{
    for (auto& t : entry_tasks)
        t->addDependent(*finalizing_task);
}

void RenderingTasks::run()
{
    m_task_sink.run();
}
