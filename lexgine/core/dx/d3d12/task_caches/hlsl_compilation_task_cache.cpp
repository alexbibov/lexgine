#include "hlsl_compilation_task_cache.h"
#include "lexgine/core/misc/strict_weak_ordering.h"
#include "lexgine/core/globals.h"

using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12::task_caches;
using namespace lexgine::core::dx::d3d12::tasks;

HLSLCompilationTaskCache::HLSLCompilationTaskCache()
{
}

void HLSLCompilationTaskCache::addTask(core::Globals& globals, std::string const& source, std::string const& source_name, 
    dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point, 
    ShaderSourceCodePreprocessor::SourceType source_type,
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions, 
    dxcompilation::HLSLCompilationOptimizationLevel optimization_level, 
    bool strict_mode, bool force_all_resources_be_bound, 
    bool force_ieee_standard, bool treat_warnings_as_errors, bool enable_validation,
    bool enable_debug_information, bool enable_16bit_types)
{
    if (m_tasks_cache_keys.insert(misc::HashedString{ source }).second)
    {
        m_tasks.emplace_back(
            globals, source, source_name, shader_model, shader_type,
            shader_entry_point, source_type,
            macro_definitions, optimization_level,
            strict_mode, force_all_resources_be_bound,
            force_ieee_standard, treat_warnings_as_errors,
            enable_validation, enable_debug_information, enable_16bit_types);
    }
}

HLSLCompilationTaskCache::iterator HLSLCompilationTaskCache::begin()
{
    return m_tasks.begin();
}

HLSLCompilationTaskCache::iterator HLSLCompilationTaskCache::end()
{
    return m_tasks.end();
}

HLSLCompilationTaskCache::const_iterator HLSLCompilationTaskCache::begin() const
{
    return m_tasks.begin();
}

HLSLCompilationTaskCache::const_iterator HLSLCompilationTaskCache::end() const
{
    return m_tasks.end();
}

size_t HLSLCompilationTaskCache::size() const
{
    return m_tasks.size();
}
