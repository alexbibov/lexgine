#ifndef LEXGINE_CORE_DX_D3D12_TASK_CACHES_HLSL_COMPILATION_TASK_CACHE_H

#include "lexgine/core/misc/hashed_string.h"
#include "lexgine/core/dx/d3d12/tasks/hlsl_compilation_task.h"

#include <list>
#include <set>


namespace lexgine {namespace core {namespace dx {namespace d3d12 {namespace task_caches {

//! Dictionary of HLSL compilation tasks
class HLSLCompilationTaskCache
{
public:
    using iterator = std::list<tasks::HLSLCompilationTask>::iterator;
    using const_iterator = std::list<tasks::HLSLCompilationTask>::const_iterator;

    HLSLCompilationTaskCache();

    void addTask(core::Globals& globals, std::string const& source, std::string const& source_name,
        dxcompilation::ShaderModel shader_model, dxcompilation::ShaderType shader_type, std::string const& shader_entry_point,
        ShaderSourceCodePreprocessor::SourceType source_type,
        std::list<dxcompilation::HLSLMacroDefinition> const& macro_definitions = std::list<dxcompilation::HLSLMacroDefinition>{},
        dxcompilation::HLSLCompilationOptimizationLevel optimization_level = dxcompilation::HLSLCompilationOptimizationLevel::level3,
        bool strict_mode = true, bool force_all_resources_be_bound = false,
        bool force_ieee_standard = true, bool treat_warnings_as_errors = true, bool enable_validation = true,
        bool enable_debug_information = false, bool enable_16bit_types = false);

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    size_t size() const;

private:
    std::list<tasks::HLSLCompilationTask> m_tasks;
    std::set<misc::HashedString> m_tasks_cache_keys;
};


}}}}}

#define LEXGINE_CORE_DX_D3D12_TASK_CACHES_HLSL_COMPILATION_TASK_CACHE_H
#endif