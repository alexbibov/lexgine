#include "hlsl_compilation_task_cache.h"

using namespace lexgine::core::dx::d3d12::task_caches;
using namespace lexgine::core::dx::d3d12::tasks;

HLSLCompilationTaskCache::HLSLCompilationTaskCache()
{
}

HLSLCompilationTaskCache::iterator HLSLCompilationTaskCache::begin()
{
    return m_task_list.begin();
}

HLSLCompilationTaskCache::iterator HLSLCompilationTaskCache::end()
{
    return m_task_list.end();
}

HLSLCompilationTaskCache::const_iterator HLSLCompilationTaskCache::begin() const
{
    return m_task_list.begin();
}

HLSLCompilationTaskCache::const_iterator HLSLCompilationTaskCache::end() const
{
    return m_task_list.end();
}

size_t lexgine::core::dx::d3d12::task_caches::HLSLCompilationTaskCache::size() const
{
    return m_task_list.size();
}
