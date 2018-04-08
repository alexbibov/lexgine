#include "globals.h"
#include "global_settings.h"
#include "lexgine/core/dx/d3d12/dx_resource_factory.h"
#include "lexgine/core/dx/d3d12/task_caches/hlsl_compilation_task_cache.h"
#include "lexgine/core/dx/d3d12/task_caches/pso_compilation_task_cache.h"
#include "lexgine/core/dx/d3d12/task_caches/root_signature_compilation_task_cache.h"

#include <cassert>

using namespace lexgine::core;



void* Globals::find(misc::HashedString const& hashed_name)
{
    global_object_pool_type::iterator target_entry;
    if ((target_entry = m_global_object_pool.find(hashed_name)) == m_global_object_pool.end()) return nullptr;

    return target_entry->second;
}

void const* Globals::find(misc::HashedString const& hashed_name) const
{
    return const_cast<Globals*>(this)->find(hashed_name);
}

bool Globals::put(misc::HashedString const& hashed_name, void* p_object)
{
    return m_global_object_pool.insert(std::make_pair(hashed_name, p_object)).second;
}




void MainGlobalsBuilder::defineGlobalSettings(GlobalSettings& global_settings)
{
    m_global_settings = &global_settings;
}

void MainGlobalsBuilder::registerWorkerThreadLogs(std::vector<std::ostream*>& worker_threads_logging_output_streams)
{
    assert(m_global_settings);
    uint8_t num_workers = m_global_settings->getNumberOfWorkers();
    assert(worker_threads_logging_output_streams.size() == num_workers);

    m_worker_logs = &worker_threads_logging_output_streams;
}

void MainGlobalsBuilder::registerMainLog(std::ostream& logging_output_stream)
{
    m_main_log = &logging_output_stream;
}

void MainGlobalsBuilder::registerDxResourceFactory(dx::d3d12::DxResourceFactory& dx_resource_factory)
{
    m_dx_resource_factory = &dx_resource_factory;
}

void MainGlobalsBuilder::registerHLSLCompilationTaskCache(dx::d3d12::task_caches::HLSLCompilationTaskCache& shader_cache)
{
    m_shader_cache = &shader_cache;
}

void MainGlobalsBuilder::registerPSOCompilationTaskCache(dx::d3d12::task_caches::PSOCompilationTaskCache& pso_cache)
{
    m_pso_cache = &pso_cache;
}

void MainGlobalsBuilder::registerRootSignatureCompilationTaskCache(dx::d3d12::task_caches::RootSignatureCompilationTaskCache& rs_cache)
{
    m_rs_cache = &rs_cache;
}

Globals MainGlobalsBuilder::build()
{
    Globals rv;
    rv.put(m_global_settings);
    rv.put(m_worker_logs);
    rv.put(m_main_log);
    rv.put(m_dx_resource_factory);
    rv.put(m_shader_cache);
    rv.put(m_pso_cache);
    rv.put(m_rs_cache);

    return rv;
}
