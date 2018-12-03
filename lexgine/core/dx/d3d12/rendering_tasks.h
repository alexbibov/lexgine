#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/concurrency/task_sink.h"

#include "lexgine_core_dx_d3d12_fwd.h"


namespace lexgine::core::dx::d3d12 {

class RenderingTasks 
{
public:
    RenderingTasks(Globals const& globals, 
        std::set<concurrency::TaskGraphRootNode*> const& entry_tasks,
        concurrency::TaskGraphNode* finalizing_task);

    void run();

private:
    Globals const& m_globals;
    concurrency::TaskGraph m_task_graph;
    concurrency::TaskSink m_task_sink;
    concurrency::TaskGraphNode* m_finalizing_task_ptr;
};

}

#endif // !LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

