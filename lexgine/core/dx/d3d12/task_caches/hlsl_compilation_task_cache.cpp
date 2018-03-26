#include "hlsl_compilation_task_cache.h"
#include "lexgine/core/misc/strict_weak_ordering.h"

using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12::task_caches;
using namespace lexgine::core::dx::d3d12::tasks;

HLSLCompilationTaskCache::HLSLCompilationTaskCache()
{
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

size_t lexgine::core::dx::d3d12::task_caches::HLSLCompilationTaskCache::size() const
{
    return m_tasks.size();
}

bool HLSLCompilationTaskCache::HLSLCompilationTaskCacheEntryComparator::operator()(HLSLCompilationTaskCacheEntry const& a, HLSLCompilationTaskCacheEntry const& b) const
{
    SWO_STEP(a.m_hash.hash(), < , b.m_hash.hash());
    SWO_END(a.m_hash.string(), < , b.m_hash.string());
}

HLSLCompilationTaskCache::HLSLCompilationTaskCacheEntry::HLSLCompilationTaskCacheEntry(
    core::Globals const& globals, 
    std::string const& source, std::string const& source_name, 
    dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, 
    std::string const& shader_entry_point, 
    ShaderSourceCodePreprocessor::SourceType source_type, 
    void* p_target_pso_descriptors, uint32_t num_descriptors, 
    std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions, 
    dxcompilation::HLSLCompilationOptimizationLevel optimization_level, 
    bool strict_mode, bool force_all_resources_be_bound, bool force_ieee_standard, 
    bool treat_warnings_as_errors, bool enable_validation, bool enable_debug_information, 
    bool enable_16bit_types):
    m_task{ globals, source, source_name, shader_model, shader_type, shader_entry_point,
source_type, p_target_pso_descriptors, num_descriptors, macro_definitions, optimization_level,
strict_mode, force_all_resources_be_bound, force_ieee_standard, treat_warnings_as_errors, enable_validation,
enable_debug_information, enable_16bit_types },
m_hash{ source }
{
}
