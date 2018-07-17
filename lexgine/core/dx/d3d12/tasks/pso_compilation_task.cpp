#include "pso_compilation_task.h"
#include "hlsl_compilation_task.h"
#include "root_signature_compilation_task.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/dx/d3d12/task_caches/cache_utilities.h"

#include <d3dcompiler.h>


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks;


namespace {

D3DDataBlob loadPrecachedPSOBlob(GlobalSettings const& global_settings, task_caches::CombinedCacheKey const& key)
{
    auto pso_cache_containinig_requested_pso =
        task_caches::findCombinedCacheContainingKey(key, global_settings);


    D3DDataBlob rv{ nullptr };
    if (pso_cache_containinig_requested_pso.isValid())
    {
        SharedDataChunk blob =
            static_cast<task_caches::StreamedCacheConnection&>(pso_cache_containinig_requested_pso).cache.retrieveEntry(key);

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
    GlobalSettings const& global_settings,
    Device& device,
    GraphicsPSODescriptor const& descriptor):
    m_key{ key },
    m_device{ device },
    m_global_settings{ global_settings },
    m_descriptor{ descriptor },
    m_associated_shader_compilation_tasks(5, nullptr),
    m_associated_root_signature{ nullptr },
    m_was_successful{ false },
    m_resulting_pipeline_state{ nullptr }
{
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
    return do_task(worker_id, 0);
}

void GraphicsPSOCompilationTask::setVertexShaderCompilationTask(HLSLCompilationTask* vs_compilation_task)
{
    this->addDependent(*vs_compilation_task);
    m_associated_shader_compilation_tasks[0] = vs_compilation_task;
}

HLSLCompilationTask* GraphicsPSOCompilationTask::getVertexShaderCompilationTask() const
{
    return m_associated_shader_compilation_tasks[0];
}

void GraphicsPSOCompilationTask::setHullShaderCompilationTask(HLSLCompilationTask* hs_compilation_task)
{
    this->addDependent(*hs_compilation_task);
    m_associated_shader_compilation_tasks[1] = hs_compilation_task;
}

HLSLCompilationTask* GraphicsPSOCompilationTask::getHullShaderCompilationTask() const
{
    return m_associated_shader_compilation_tasks[1];
}

void GraphicsPSOCompilationTask::setDomainShaderCompilationTask(HLSLCompilationTask* ds_compilation_task)
{
    this->addDependent(*ds_compilation_task);
    m_associated_shader_compilation_tasks[2] = ds_compilation_task;
}

HLSLCompilationTask* GraphicsPSOCompilationTask::getDomainShaderCompilationTask() const
{
    return m_associated_shader_compilation_tasks[2];
}

void GraphicsPSOCompilationTask::setGeometryShaderCompilationTask(HLSLCompilationTask* gs_compilation_task)
{
    this->addDependent(*gs_compilation_task);
    m_associated_shader_compilation_tasks[3] = gs_compilation_task;
}

HLSLCompilationTask* GraphicsPSOCompilationTask::getGeometryShaderCompilationTask() const
{
    return m_associated_shader_compilation_tasks[3];
}

void GraphicsPSOCompilationTask::setPixelShaderCompilationTask(HLSLCompilationTask* ps_compilation_task)
{
    this->addDependent(*ps_compilation_task);
    m_associated_shader_compilation_tasks[4] = ps_compilation_task;
}

HLSLCompilationTask* GraphicsPSOCompilationTask::getPixelShaderCompilationTask() const
{
    return m_associated_shader_compilation_tasks[4];
}

void GraphicsPSOCompilationTask::setRootSignatureCompilationTask(RootSignatureCompilationTask* root_signature_compilation_task)
{
    this->addDependent(*root_signature_compilation_task);
    m_associated_root_signature = root_signature_compilation_task;
}

RootSignatureCompilationTask* GraphicsPSOCompilationTask::getRootSignatureCompilationTask() const
{
    return m_associated_root_signature;
}

std::string GraphicsPSOCompilationTask::getCacheName() const
{
    return m_key.toString();
}

bool GraphicsPSOCompilationTask::do_task(uint8_t worker_id, uint16_t frame_index)
{
    try
    {
        auto precached_pso_blob = loadPrecachedPSOBlob(m_global_settings, m_key);

        m_resulting_pipeline_state.reset(new PipelineState{
            m_device,
            m_associated_root_signature->getTaskData(),
            getCacheName(),
            m_descriptor, precached_pso_blob });

        if (!precached_pso_blob)
        {
            auto my_pso_cache =
                task_caches::establishConnectionWithCombinedCache(m_global_settings, worker_id, false);

            static_cast<task_caches::StreamedCacheConnection&>(my_pso_cache).cache.addEntry(
                task_caches::CombinedCache::entry_type{ m_key, m_resulting_pipeline_state->getCache() }
            );
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

concurrency::TaskType GraphicsPSOCompilationTask::get_task_type() const
{
    return concurrency::TaskType::cpu;
}



ComputePSOCompilationTask::ComputePSOCompilationTask(
    task_caches::CombinedCacheKey const& key,
    GlobalSettings const& global_settings,
    Device& device,
    ComputePSODescriptor const& descriptor):
    m_key{ key },
    m_global_settings{ global_settings },
    m_device{ device },
    m_descriptor{ descriptor }
{

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
    return do_task(worker_id, 0);
}

void ComputePSOCompilationTask::setComputeShaderCompilationTask(HLSLCompilationTask* cs_compilation_task)
{
    this->addDependent(*cs_compilation_task);
}

void ComputePSOCompilationTask::setRootSignatureCompilationTask(RootSignatureCompilationTask* root_signature_compilation_task)
{
    this->addDependent(*root_signature_compilation_task);
}

std::string ComputePSOCompilationTask::getCacheName() const
{
    return m_key.toString();
}

bool ComputePSOCompilationTask::do_task(uint8_t worker_id, uint16_t frame_index)
{
    try
    {
        auto precached_pso_blob = loadPrecachedPSOBlob(m_global_settings, m_key);

        m_resulting_pipeline_state.reset(new PipelineState{
            m_device,
            m_associated_root_signature->getTaskData(),
            getCacheName(),
            m_descriptor, precached_pso_blob });

        if (!precached_pso_blob)
        {
            auto my_pso_cache =
                task_caches::establishConnectionWithCombinedCache(m_global_settings, worker_id, false);

            static_cast<task_caches::StreamedCacheConnection&>(my_pso_cache).cache.addEntry(
                task_caches::CombinedCache::entry_type{ m_key, m_resulting_pipeline_state->getCache() }
            );
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

concurrency::TaskType ComputePSOCompilationTask::get_task_type() const
{
    return concurrency::TaskType::cpu;
}
