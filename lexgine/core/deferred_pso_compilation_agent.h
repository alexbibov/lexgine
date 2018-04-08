#ifndef LEXGINE_CORE_DEFERRED_PSO_COMPILATION_AGENT_H
#define LEXGINE_CORE_DEFERRED_PSO_COMPILATION_AGENT_H

#include "lexgine/core/dx/d3d12/task_caches/lexgine_core_dx_d3d12_task_caches_fwd.h"

namespace lexgine { namespace core {

class DeferredPSOCompilationAgent
{
public:
    DeferredPSOCompilationAgent(dx::d3d12::task_caches::PSOCompilationTaskCache const& pso_cache);

    void compile() const;
    bool isReady() const;

private:
    class DeferredPSOCompilationAgentExitTask;

private:
    dx::d3d12::task_caches::PSOCompilationTaskCache const& m_pso_cache;
    bool m_is_completed;
};

}}

#endif
