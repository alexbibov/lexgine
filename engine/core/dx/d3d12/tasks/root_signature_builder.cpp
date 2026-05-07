#include "root_signature_builder.h"
#include "engine/core/exception.h"
#include "engine/core/globals.h"
#include "engine/core/misc/log.h"

#include "engine/core/dx/d3d12/task_caches/data_cache.h"

#include <d3dcompiler.h>

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks;
using namespace lexgine::core::dx::d3d12::task_caches;

RootSignatureBuilder::RootSignatureBuilder(
    task_caches::CombinedCacheKey const& key,
    Globals& globals, RootSignature&& root_signature, RootSignatureFlags const& flags, misc::DateTime const& timestamp)
    : m_key{ key }
    , m_globals{ globals }
    , m_rs{ std::move(root_signature) }
    , m_rs_flags{ flags }
    , m_was_successful{ false }
    , m_timestamp{ timestamp }
{
}

D3DDataBlob const& RootSignatureBuilder::getTaskData() const
{
    return m_compiled_rs_blob;
}

bool RootSignatureBuilder::wasSuccessful() const
{
    return m_was_successful;
}

std::string RootSignatureBuilder::getCacheName() const
{
    return m_key.toString();
}

bool RootSignatureBuilder::build(uint8_t worker_id)
{
    (void) worker_id;

    if (m_was_successful)
        return true;

    try
    {
        D3DDataBlob rs_blob{ nullptr };
        SharedDataChunk cached_rs_blob{};
        auto rs_cache = m_globals.get<task_caches::DataCache>();

        if (rs_cache && *rs_cache)
        {
            auto cache_access = rs_cache->cache().access();
            if (cache_access->doesEntryExist(m_key) && cache_access->getEntryTimestamp(m_key) >= m_timestamp)
                cached_rs_blob = cache_access->retrieveEntry(m_key);
        }

        if (cached_rs_blob.size() && cached_rs_blob.data())
        {
            Microsoft::WRL::ComPtr<ID3DBlob> d3d_blob{ nullptr };
            HRESULT hres = D3DCreateBlob(cached_rs_blob.size(), d3d_blob.GetAddressOf());
            if (hres == S_OK || hres == S_FALSE)
            {
                memcpy(d3d_blob->GetBufferPointer(), cached_rs_blob.data(), cached_rs_blob.size());
                rs_blob = D3DDataBlob{ d3d_blob };
            }
        }

        if (!rs_blob)
        {
            m_compiled_rs_blob = m_rs.compile(m_rs_flags);
            m_was_successful = !m_rs.getErrorState();

            if (m_was_successful)
            {
                if (rs_cache && *rs_cache)
                {
                    rs_cache->cache()->addEntry(task_caches::CombinedCache::entry_type{ m_key, m_compiled_rs_blob });
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
        misc::Log::retrieve()->out(std::string{ "RootSignatureBuilder \"" } + getCacheName() + "\": LEXGINE has thrown exception: " + e.what(), misc::LogMessageType::error);
        m_was_successful = false;
    }
    catch (...)
    {
        misc::Log::retrieve()->out(std::string{ "RootSignatureBuilder \"" } + getCacheName() + "\": unrecognized exception", misc::LogMessageType::error);
        m_was_successful = false;
    }

    return m_was_successful;
}
