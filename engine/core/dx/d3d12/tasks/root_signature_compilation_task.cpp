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

RootSignatureCompilationTask::RootSignatureCompilationTask(
    task_caches::CombinedCacheKey const& key,
    Globals const& globals, RootSignature&& root_signature, RootSignatureFlags const& flags, misc::DateTime const& timestamp)
    : SchedulableTask{ static_cast<RootSignatureCompilationTaskCache::Key>(key).rs_cache_name, true }
    , m_key{ key }
    , m_global_settings{ *globals.get<GlobalSettings>() }
    , m_rs{ std::move(root_signature) }
    , m_rs_flags{ flags }
    , m_was_successful{ false }
    , m_timestamp{ timestamp }
{
    addProfilingService(std::make_unique<CPUTaskProfilingService>(*globals.get<GlobalSettings>(), getStringName()));
}

D3DDataBlob const& RootSignatureCompilationTask::getTaskData() const
{
    return m_compiled_rs_blob;
}

bool RootSignatureCompilationTask::wasSuccessful() const
{
    return m_was_successful;
}

bool RootSignatureCompilationTask::execute(uint8_t worker_id)
{
    return doTask(worker_id, 0);
}

std::string RootSignatureCompilationTask::getCacheName() const
{
    return m_key.toString();
}

bool RootSignatureCompilationTask::doTask(uint8_t worker_id, uint64_t)
{
    try
    {
        auto rs_cache_with_requested_root_signature =
            task_caches::findCombinedCacheContainingKey(m_key, m_global_settings);


        D3DDataBlob rs_blob{ nullptr };
        if (rs_cache_with_requested_root_signature.isValid()
            && static_cast<task_caches::StreamedCacheConnection&>(rs_cache_with_requested_root_signature)
            .cache().getEntryTimestamp(m_key) >= m_timestamp)
        {
            SharedDataChunk blob =
                static_cast<task_caches::StreamedCacheConnection&>(rs_cache_with_requested_root_signature)
                .cache().retrieveEntry(m_key);

            if (blob.size() && blob.data())
            {
                Microsoft::WRL::ComPtr<ID3DBlob> d3d_blob{ nullptr };
                HRESULT hres = D3DCreateBlob(blob.size(), d3d_blob.GetAddressOf());
                if (hres == S_OK || hres == S_FALSE)
                {
                    memcpy(d3d_blob->GetBufferPointer(), blob.data(), blob.size());
                    rs_blob = D3DDataBlob{ d3d_blob };
                }
            }
        }

        if (!rs_blob)
        {
            m_compiled_rs_blob = m_rs.compile(m_rs_flags);
            m_was_successful = !m_rs.getErrorState();

            if (m_was_successful)
            {
                auto my_rs_cache = task_caches::establishConnectionWithCombinedCache(m_global_settings, worker_id, false);
                if (my_rs_cache.isValid())
                {
                    static_cast<task_caches::StreamedCacheConnection&>(my_rs_cache).cache().addEntry(task_caches::CombinedCache::entry_type{ m_key, m_compiled_rs_blob });
                }
            }

        }
        else
        {
            m_compiled_rs_blob = rs_blob;
            m_was_successful = true;
        }

    }
    catch (Exception const& e)
    {
        LEXGINE_LOG_ERROR(this, std::string{ "LEXGINE has thrown exception: " } +e.what());
        m_was_successful = false;
    }
    catch (...)
    {
        LEXGINE_LOG_ERROR(this, "unrecognized exception");
        m_was_successful = false;
    }

    return true;    // under no circumstances this task can be rescheduled
}

concurrency::TaskType RootSignatureCompilationTask::type() const
{
    return concurrency::TaskType::cpu;
}
