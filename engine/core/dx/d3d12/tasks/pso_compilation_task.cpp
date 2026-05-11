#include "pso_compilation_task.h"
#include "hlsl_compilation_task.h"
#include "root_signature_builder.h"
#include "engine/core/exception.h"
#include "engine/core/globals.h"
#include "engine/core/global_settings.h"
#include "engine/core/profiling_services.h"
#include "engine/core/data_cache.h"

#include <d3dcompiler.h>


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks;
using namespace lexgine::core::dx::d3d12::task_caches;


namespace {

D3DDataBlob loadPrecachedPSOBlob(Globals const& globals, GpuDataBlobCacheKey const& key,
    misc::DateTime const& timestamp)
{
    D3DDataBlob rv{ nullptr };
    SharedDataChunk cached_pso_blob{};
    auto pso_cache = globals.get<DataCache>();

    if (pso_cache && *pso_cache)
    {
        auto cache_access = pso_cache->cache().access();
        if (cache_access->doesEntryExist(key) && cache_access->getEntryTimestamp(key) >= timestamp)
            cached_pso_blob = cache_access->retrieveEntry(key);
    }

    if (cached_pso_blob.size() && cached_pso_blob.data())
    {
        Microsoft::WRL::ComPtr<ID3DBlob> d3d_blob{ nullptr };
        HRESULT res = D3DCreateBlob(cached_pso_blob.size(), d3d_blob.GetAddressOf());
        if (res == S_OK || res == S_FALSE)
        {
            assert(d3d_blob->GetBufferSize() >= cached_pso_blob.size());
            memcpy(d3d_blob->GetBufferPointer(), cached_pso_blob.data(), cached_pso_blob.size());
            rv = D3DDataBlob{ d3d_blob };
        }
    }

    return rv;
}

}


GraphicsPSOCompilationTask::GraphicsPSOCompilationTask(
    GpuDataBlobCacheKey const& key,
    Globals& globals,
    GraphicsPSODescriptor const& descriptor, misc::DateTime const& timestamp)
    : SchedulableTask{ static_cast<PSOCompilationTaskCache::Key>(key).pso_cache_name, true }
    , m_key{ key }
    , m_globals{ globals }
    , m_descriptor{ descriptor }
    , m_associated_shader_compilation_tasks{ 0 }
    , m_associated_root_signature_builder{ nullptr }
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

void GraphicsPSOCompilationTask::setRootSignatureBuilder(RootSignatureBuilder* root_signature_builder)
{
    m_associated_root_signature_builder = root_signature_builder;
}

RootSignatureBuilder* GraphicsPSOCompilationTask::getRootSignatureBuilder() const
{
    return m_associated_root_signature_builder;
}

std::string GraphicsPSOCompilationTask::getCacheName() const
{
    return m_key.toString();
}

bool GraphicsPSOCompilationTask::doTask(uint8_t worker_id, uint64_t)
{
    (void) worker_id;

    try
    {
        if (m_associated_root_signature_builder && !m_root_signature)
            m_associated_root_signature_builder->build(worker_id);

        auto precached_pso_blob = loadPrecachedPSOBlob(m_globals, m_key, m_timestamp);

        if (!m_descriptor.vertex_shader)
        {
            m_descriptor.vertex_shader = m_associated_shader_compilation_tasks[0]->getTaskData();
        }

        if (!m_descriptor.hull_shader && m_associated_shader_compilation_tasks[1])
            m_descriptor.hull_shader = m_associated_shader_compilation_tasks[1]->getTaskData();

        if (!m_descriptor.domain_shader && m_associated_shader_compilation_tasks[2])
            m_descriptor.domain_shader = m_associated_shader_compilation_tasks[2]->getTaskData();

        if (!m_descriptor.geometry_shader && m_associated_shader_compilation_tasks[3])
            m_descriptor.geometry_shader = m_associated_shader_compilation_tasks[3]->getTaskData();

        if (!m_descriptor.pixel_shader)
        {
            m_descriptor.pixel_shader = m_associated_shader_compilation_tasks[4]->getTaskData();
        }

        m_resulting_pipeline_state.reset(new PipelineState{
            m_globals,
            m_root_signature ? m_root_signature : m_associated_root_signature_builder->getTaskData(),
            m_root_signature ? getCacheName() : m_associated_root_signature_builder->getCacheName(),
            m_descriptor, precached_pso_blob });

        if (!precached_pso_blob)
        {
            auto my_pso_cache = m_globals.get<DataCache>();
            if (my_pso_cache && *my_pso_cache)
            {
                my_pso_cache->cache()->addEntry(CombinedCache::entry_type{ m_key, m_resulting_pipeline_state->getCache() });
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
    GpuDataBlobCacheKey const& key,
    Globals& globals,
    ComputePSODescriptor const& descriptor, misc::DateTime const& timestamp)
    : SchedulableTask{ static_cast<PSOCompilationTaskCache::Key>(key).pso_cache_name, true }
    , m_key{ key }
    , m_globals{ globals }
    , m_descriptor{ descriptor }
    , m_associated_compute_shader_compilation_task{ nullptr }
    , m_associated_root_signature_builder{ nullptr }
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

void ComputePSOCompilationTask::setRootSignatureBuilder(RootSignatureBuilder* root_signature_builder)
{
    m_associated_root_signature_builder = root_signature_builder;
}

RootSignatureBuilder* ComputePSOCompilationTask::getRootSignatureBuilder() const
{
    return m_associated_root_signature_builder;
}

std::string ComputePSOCompilationTask::getCacheName() const
{
    return m_key.toString();
}

bool ComputePSOCompilationTask::doTask(uint8_t worker_id, uint64_t)
{
    (void) worker_id;

    try
    {
        if (m_associated_root_signature_builder && !m_root_signature)
            m_associated_root_signature_builder->build(worker_id);

        auto precached_pso_blob = loadPrecachedPSOBlob(m_globals, m_key, m_timestamp);

        if (!m_descriptor.compute_shader)
        {
            m_descriptor.compute_shader = m_associated_compute_shader_compilation_task->getTaskData();
        }

        m_resulting_pipeline_state.reset(new PipelineState{
            m_globals,
            m_root_signature ? m_root_signature : m_associated_root_signature_builder->getTaskData(),
            m_root_signature ? getCacheName() : m_associated_root_signature_builder->getCacheName(),
            m_descriptor, precached_pso_blob });

        if (!precached_pso_blob)
        {
            auto my_pso_cache = m_globals.get<DataCache>();
            if (my_pso_cache && *my_pso_cache)
            {
                my_pso_cache->cache()->addEntry(CombinedCache::entry_type{ m_key, m_resulting_pipeline_state->getCache() });
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
