#include "root_signature_compilation_task.h"
#include "lexgine/core/exception.h"

#include "lexgine/core/dx/d3d12/task_caches/cache_utilities.h"

#include <d3dcompiler.h>


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks;


RootSignatureCompilationTask::RootSignatureCompilationTask(
    task_caches::CombinedCacheKey const& key, 
    GlobalSettings const& global_settings,
    RootSignature&& root_signature, RootSignatureFlags const& flags):
    m_key{ key },
    m_global_settings{ global_settings },
    m_rs{ std::move(root_signature) },
    m_rs_flags{ flags },
    m_was_successful{ false }
{
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
    return do_task(worker_id, 0);
}

std::string RootSignatureCompilationTask::getCacheName() const
{
    return m_key.toString();
}

bool RootSignatureCompilationTask::do_task(uint8_t worker_id, uint16_t frame_index)
{
    try
    {
        auto rs_cache_with_requested_root_signature =
            task_caches::findCombinedCacheContainingKey(m_key, m_global_settings);


        D3DDataBlob rs_blob{ nullptr };
        if (rs_cache_with_requested_root_signature.isValid())
        {
            SharedDataChunk blob =
                static_cast<task_caches::StreamedCacheConnection&>(rs_cache_with_requested_root_signature).cache().retrieveEntry(m_key);

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
            m_was_successful = m_rs.getErrorState();

            if (m_was_successful)
            {
                auto my_rs_cache =
                    task_caches::establishConnectionWithCombinedCache(m_global_settings, worker_id, false);

                my_rs_cache.cache().addEntry(task_caches::CombinedCache::entry_type{ m_key, m_compiled_rs_blob });
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
        LEXGINE_LOG_ERROR(this, std::string{ "LEXGINE has thrown exception: " } + e.what());
        m_was_successful = false;
    }
    catch (...)
    {
        LEXGINE_LOG_ERROR(this, "unrecognized exception");
        m_was_successful = false;
    }

    return true;    // under no circumstances this task can be rescheduled
}

concurrency::TaskType RootSignatureCompilationTask::get_task_type() const
{
    return concurrency::TaskType::cpu;
}
