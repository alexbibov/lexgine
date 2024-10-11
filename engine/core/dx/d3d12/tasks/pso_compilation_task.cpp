#include "pso_compilation_task.h"
#include "hlsl_compilation_task.h"
#include "root_signature_compilation_task.h"
#include "engine/core/exception.h"
#include "engine/core/globals.h"
#include "engine/core/global_settings.h"
#include "engine/core/profiling_services.h"
#include "engine/core/dx/d3d12/task_caches/cache_utilities.h"

#include <d3dcompiler.h>


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks;
using namespace lexgine::core::dx::d3d12::task_caches;


namespace {

D3DDataBlob loadPrecachedPSOBlob(GlobalSettings const& global_settings, task_caches::CombinedCacheKey const& key,
    misc::DateTime const& timestamp)
{
    auto pso_cache_containinig_requested_pso =
        task_caches::findCombinedCacheContainingKey(key, global_settings);


    D3DDataBlob rv{ nullptr };

    if (pso_cache_containinig_requested_pso.isValid() 
        && pso_cache_containinig_requested_pso->cache().getEntryTimestamp(key) >= timestamp)
    {
        SharedDataChunk blob = pso_cache_containinig_requested_pso->cache().retrieveEntry(key);

        if (blob.size() && blob.data())
        {
            Microsoft::WRL::ComPtr<ID3DBlob> d3d_blob{ nullptr };
            HRESULT res = D3DCreateBlob(blob.size(), d3d_blob.GetAddressOf());
            if (res == S_OK || res == S_FALSE)
            {
                memcpy(d3d_blob->GetBufferPointer(), blob.data(), blob.size());
                rv = D3DDataBlob{ d3d_blob };
            }
        }
    }

    return rv;
}

}


GraphicsPSOCompilationTask::GraphicsPSOCompilationTask(
    task_caches::CombinedCacheKey const& key,
    Globals& globals,
    GraphicsPSODescriptor const& descriptor, misc::DateTime const& timestamp)
    : SchedulableTask{ static_cast<PSOCompilationTaskCache::Key>(key).pso_cache_name, true }
    , m_key{ key }
    , m_globals{ globals }
    , m_descriptor{ descriptor }
    , m_associated_shader_compilation_tasks(5, nullptr)
    , m_associated_root_signature_compilation_task{ nullptr }
    , m_was_successful{ false }
    , m_resulting_pipeline_state{ nullptr }
    , m_timestamp{ timestamp }
{
    addProfilingService(std::make_unique<CPUTaskProfilingService>(*globals.get<GlobalSettings>(), getStringName()));
}

PipelineState const& GraphicsPSOCompilationTask::getTaskData() const
{
    return *m_resulting_pipeline_state;
}

bool GraphicsPSOCompilationTask::wasSuccessful() const
{
    return m_was_successful;
}

bool GraphicsPSOCompilationTask::execute(uint8_t worker_id)
{
    return doTask(worker_id, 0);
}

void GraphicsPSOCompilationTask::setVertexShaderCompilationTask(HLSLCompilationTask* vs_compilation_task)
{
    this->addDependency(*vs_compilation_task);
    m_associated_shader_compilation_tasks[0] = vs_compilation_task;
}

HLSLCompilationTask* GraphicsPSOCompilationTask::getVertexShaderCompilationTask() const
{
    return m_associated_shader_compilation_tasks[0];
}

void GraphicsPSOCompilationTask::setHullShaderCompilationTask(HLSLCompilationTask* hs_compilation_task)
{
    this->addDependency(*hs_compilation_task);
    m_associated_shader_compilation_tasks[1] = hs_compilation_task;
}

HLSLCompilationTask* GraphicsPSOCompilationTask::getHullShaderCompilationTask() const
{
    return m_associated_shader_compilation_tasks[1];
}

void GraphicsPSOCompilationTask::setDomainShaderCompilationTask(HLSLCompilationTask* ds_compilation_task)
{
    this->addDependency(*ds_compilation_task);
    m_associated_shader_compilation_tasks[2] = ds_compilation_task;
}

HLSLCompilationTask* GraphicsPSOCompilationTask::getDomainShaderCompilationTask() const
{
    return m_associated_shader_compilation_tasks[2];
}

void GraphicsPSOCompilationTask::setGeometryShaderCompilationTask(HLSLCompilationTask* gs_compilation_task)
{
    this->addDependency(*gs_compilation_task);
    m_associated_shader_compilation_tasks[3] = gs_compilation_task;
}

HLSLCompilationTask* GraphicsPSOCompilationTask::getGeometryShaderCompilationTask() const
{
    return m_associated_shader_compilation_tasks[3];
}

void GraphicsPSOCompilationTask::setPixelShaderCompilationTask(HLSLCompilationTask* ps_compilation_task)
{
    this->addDependency(*ps_compilation_task);
    m_associated_shader_compilation_tasks[4] = ps_compilation_task;
}

HLSLCompilationTask* GraphicsPSOCompilationTask::getPixelShaderCompilationTask() const
{
    return m_associated_shader_compilation_tasks[4];
}

void GraphicsPSOCompilationTask::setRootSignatureCompilationTask(RootSignatureCompilationTask* root_signature_compilation_task)
{
    this->addDependency(*root_signature_compilation_task);
    m_associated_root_signature_compilation_task = root_signature_compilation_task;
}

RootSignatureCompilationTask* GraphicsPSOCompilationTask::getRootSignatureCompilationTask() const
{
    return m_associated_root_signature_compilation_task;
}

std::string GraphicsPSOCompilationTask::getCacheName() const
{
    return m_key.toString();
}

bool GraphicsPSOCompilationTask::doTask(uint8_t worker_id, uint64_t)
{
    try
    {
        auto precached_pso_blob = loadPrecachedPSOBlob(*m_globals.get<GlobalSettings>(), m_key, m_timestamp);

        m_descriptor.vertex_shader = m_associated_shader_compilation_tasks[0]->getTaskData();

        if (m_associated_shader_compilation_tasks[1])
            m_descriptor.hull_shader = m_associated_shader_compilation_tasks[1]->getTaskData();

        if (m_associated_shader_compilation_tasks[2])
            m_descriptor.domain_shader = m_associated_shader_compilation_tasks[2]->getTaskData();

        if (m_associated_shader_compilation_tasks[3])
            m_descriptor.geometry_shader = m_associated_shader_compilation_tasks[3]->getTaskData();

        m_descriptor.pixel_shader = m_associated_shader_compilation_tasks[4]->getTaskData();

        m_resulting_pipeline_state.reset(new PipelineState{
            m_globals,
            m_associated_root_signature_compilation_task->getTaskData(),
            m_associated_root_signature_compilation_task->getCacheName(),
            m_descriptor, precached_pso_blob });

        if (!precached_pso_blob)
        {
            auto my_pso_cache = task_caches::establishConnectionWithCombinedCache(*m_globals.get<GlobalSettings>(), worker_id, false);
            if (my_pso_cache.isValid())
            {
                my_pso_cache->cache().addEntry(task_caches::CombinedCache::entry_type{ m_key, m_resulting_pipeline_state->getCache() });
            }
        }
    }
    catch (Exception const& e)
    {
        LEXGINE_LOG_ERROR(this, std::string{ "LEXGINE has thrown exception: " } +e.what());
    }
    catch (...)
    {
        LEXGINE_LOG_ERROR(this, "unspecified exception");
        m_was_successful = false;
    }

    return true;    // this task is never rescheduled
}

concurrency::TaskType GraphicsPSOCompilationTask::type() const
{
    return concurrency::TaskType::cpu;
}



ComputePSOCompilationTask::ComputePSOCompilationTask(
    task_caches::CombinedCacheKey const& key,
    Globals& globals,
    ComputePSODescriptor const& descriptor, misc::DateTime const& timestamp)
    : SchedulableTask{ static_cast<PSOCompilationTaskCache::Key>(key).pso_cache_name, true }
    , m_key{ key }
    , m_globals{ globals }
    , m_descriptor{ descriptor }
    , m_associated_compute_shader_compilation_task{ nullptr }
    , m_associated_root_signature_compilation_task{ nullptr }
    , m_was_successful{ false }
    , m_resulting_pipeline_state{ nullptr }
    , m_timestamp{ timestamp }
{
    addProfilingService(std::make_unique<CPUTaskProfilingService>(*globals.get<GlobalSettings>(), getStringName()));
}

PipelineState const& ComputePSOCompilationTask::getTaskData() const
{
    return *m_resulting_pipeline_state;
}

bool ComputePSOCompilationTask::wasSuccessful() const
{
    return m_was_successful;
}

bool ComputePSOCompilationTask::execute(uint8_t worker_id)
{
    return doTask(worker_id, 0);
}

void ComputePSOCompilationTask::setComputeShaderCompilationTask(HLSLCompilationTask* cs_compilation_task)
{
    this->addDependency(*cs_compilation_task);
    m_associated_compute_shader_compilation_task = cs_compilation_task;
}

HLSLCompilationTask* ComputePSOCompilationTask::getComputeShaderCompilationTask() const
{
    return m_associated_compute_shader_compilation_task;
}

void ComputePSOCompilationTask::setRootSignatureCompilationTask(RootSignatureCompilationTask* root_signature_compilation_task)
{
    this->addDependency(*root_signature_compilation_task);
    m_associated_root_signature_compilation_task = root_signature_compilation_task;
}

RootSignatureCompilationTask* ComputePSOCompilationTask::getRootSignatureCompilationTask() const
{
    return m_associated_root_signature_compilation_task;
}

std::string ComputePSOCompilationTask::getCacheName() const
{
    return m_key.toString();
}

bool ComputePSOCompilationTask::doTask(uint8_t worker_id, uint64_t)
{
    try
    {
        auto precached_pso_blob = loadPrecachedPSOBlob(*m_globals.get<GlobalSettings>(), m_key, m_timestamp);

        m_descriptor.compute_shader = m_associated_compute_shader_compilation_task->getTaskData();

        m_resulting_pipeline_state.reset(new PipelineState{
            m_globals,
            m_associated_root_signature_compilation_task->getTaskData(),
            m_associated_root_signature_compilation_task->getCacheName(),
            m_descriptor, precached_pso_blob });

        if (!precached_pso_blob)
        {
            auto my_pso_cache =
                task_caches::establishConnectionWithCombinedCache(*m_globals.get<GlobalSettings>(), worker_id, false);

            if (my_pso_cache.isValid()) 
            {
                my_pso_cache->cache().addEntry(task_caches::CombinedCache::entry_type{ m_key, m_resulting_pipeline_state->getCache() });
            }
        }
    }
    catch (Exception const& e)
    {
        LEXGINE_LOG_ERROR(this, std::string{ "LEXGINE has thrown exception: " } +e.what());
    }
    catch (...)
    {
        LEXGINE_LOG_ERROR(this, "unspecified exception");
        m_was_successful = false;
    }

    return true;    // this task is never rescheduled
}

concurrency::TaskType ComputePSOCompilationTask::type() const
{
    return concurrency::TaskType::cpu;
}
