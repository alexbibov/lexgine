#include "deferred_pso_compilation_agent.h"

#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/concurrency/task_sink.h"
#include "lexgine/core/dx/d3d12/task_caches/pso_compilation_task_cache.h"

using namespace lexgine::core;


class DeferredPSOCompilationAgent::DeferredPSOCompilationAgentExitTask : public concurrency::SchedulableTask
{
public:

    DeferredPSOCompilationAgentExitTask(DeferredPSOCompilationAgent& parent) :
        SchedulableTask{ "deferred_pso_compilation_agent_task_graph_exit_op" },
        m_task_sink_ptr{ nullptr },
        m_parent{ parent }
    {

    }

    void setInput(concurrency::TaskSink* task_sink_ptr)
    {
        m_task_sink_ptr = task_sink_ptr;
    }

    bool execute_manually()
    {
        return do_task(0, 0);
    }


private:
    // required by SchedulableTask interface

    bool do_task(uint8_t worker_id, uint16_t frame_index) override
    {
        if (m_task_sink_ptr) m_task_sink_ptr->dispatchExitSignal();
        m_parent.m_is_completed = true;
    }

    concurrency::TaskType get_task_type() const override
    {
        return concurrency::TaskType::cpu;
    }

private:
    concurrency::TaskSink* m_task_sink_ptr;
    DeferredPSOCompilationAgent& m_parent;
};


DeferredPSOCompilationAgent::DeferredPSOCompilationAgent(dx::d3d12::task_caches::PSOCompilationTaskCache const& pso_cache):
    m_pso_cache{ pso_cache },
    m_is_completed{ false }
{
}

void DeferredPSOCompilationAgent::compile() const
{

}

bool DeferredPSOCompilationAgent::isReady() const
{
    return m_is_completed;
}
