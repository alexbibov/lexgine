#ifndef LEXGINE_CORE_DX_D3D12_TASK_CACHES_HLSL_COMPILATION_TASK_CACHE_H

#include "../tasks/hlsl_compilation_task.h"
#include <list>

namespace lexgine {namespace core {namespace dx {namespace d3d12 {namespace task_caches {

//! Dictionary of HLSL compilation tasks
class HLSLCompilationTaskCache
{
public:
    using iterator = std::list<tasks::HLSLCompilationTask>::iterator;
    using const_iterator = std::list<tasks::HLSLCompilationTask>::const_iterator;

    HLSLCompilationTaskCache();

    template<typename ... task_construction_arguments>
    void addTask(task_construction_arguments&& ... args)
    {
        m_task_list.emplace_back(std::move(args)...);
    }

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

private:
    std::list<tasks::HLSLCompilationTask> m_task_list;
};


}}}}}

#define LEXGINE_CORE_DX_D3D12_TASK_CACHES_HLSL_COMPILATION_TASK_CACHE_H
#endif