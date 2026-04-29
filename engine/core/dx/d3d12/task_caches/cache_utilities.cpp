#include <fstream>
#include <utility>

#include "cache_utilities.h"
#include "engine/core/globals.h"
#include "engine/core/global_settings.h"
#include "engine/core/exception.h"
#include "engine/core/misc/misc.h"


using namespace lexgine::core;
using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12::task_caches;


namespace {

std::string getCombinedCachePath(GlobalSettings const& global_settings)
{
    return global_settings.getCacheDirectory() + global_settings.getCombinedCacheName()
        + "." + global_constants::combined_cache_extra_extension;
}

}


misc::Optional<StreamedCacheConnection> lexgine::core::dx::d3d12::task_caches::establishConnectionWithCombinedCache(GlobalSettings const& global_settings, bool readonly_mode, bool allow_overwrites)
{
    return establishConnectionWithCombinedCache(global_settings, getCombinedCachePath(global_settings), readonly_mode, allow_overwrites);
}


misc::Optional<StreamedCacheConnection> lexgine::core::dx::d3d12::task_caches::establishConnectionWithCombinedCache(GlobalSettings const& global_settings, uint8_t worker_id, bool readonly_mode, bool allow_overwrites)
{
    (void) worker_id;

    return establishConnectionWithCombinedCache(global_settings, readonly_mode, allow_overwrites);
}


misc::Optional<StreamedCacheConnection> lexgine::core::dx::d3d12::task_caches::establishConnectionWithCombinedCache(GlobalSettings const& global_settings, std::string const& path_to_combined_cache, bool readonly_mode, bool allow_overwrites /* = true */)
{
    return global_settings.isCacheEnabled()
        ? misc::Optional<StreamedCacheConnection>{ StreamedCacheConnection{ global_settings, path_to_combined_cache, readonly_mode, allow_overwrites } }
        : misc::Optional<StreamedCacheConnection>{};
}


misc::Optional<StreamedCacheConnection> lexgine::core::dx::d3d12::task_caches::findCombinedCacheContainingKey(CombinedCacheKey const& key, GlobalSettings const& global_settings)
{
    auto cache_connection = establishConnectionWithCombinedCache(global_settings, true);
    if (cache_connection.isValid())
    {
        if (cache_connection->cache()->doesEntryExist(key))
            return misc::Optional<StreamedCacheConnection>{std::move(cache_connection)};
    }

    return misc::Optional<StreamedCacheConnection>{};
}



StreamedCacheConnection::StreamedCacheConnection(GlobalSettings const& global_settings, std::string const& path_to_cache,
    bool is_read_only, bool allow_overwrites/* = true*/) :
    m_stream{ nullptr },
    m_cache{ nullptr }
{
    // create stream for the cache
    bool does_cache_exist{ false };
    {
        auto cache_stream_mode = std::ios_base::in | std::ios_base::binary;
        if (!is_read_only) cache_stream_mode |= std::ios_base::out;

        does_cache_exist = doesFileExist(path_to_cache);
        if (!does_cache_exist)
        {
            if (is_read_only) return;
            cache_stream_mode |= std::ios_base::trunc;
        }

        m_stream.reset(new std::fstream{ path_to_cache, cache_stream_mode });
    }

    // create cache instance
    if (m_stream && *m_stream)
    {
        if (does_cache_exist)
        {
            m_cache.reset(new CombinedCache{ *m_stream, is_read_only });

            if (!(*m_cache->access()))
            {
                // existing cache cannot be opened, probably due to data corruption
                // try to create new cache storage, if requested opening mode is not read-only
                if (is_read_only) { m_stream.release(); return; }
                m_stream->close();
                m_stream->open(path_to_cache, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
                if (!(*m_stream)) { m_stream.release(); return; }

                m_cache.reset(new CombinedCache{ *m_stream, global_settings.getMaxCombinedCacheSize(),
                    global_constants::combined_cache_compression_level, allow_overwrites });
            }
        }
        else
        {
            m_cache.reset(new CombinedCache{ *m_stream, global_settings.getMaxCombinedCacheSize(),
                global_constants::combined_cache_compression_level, allow_overwrites });
        }
    }
}

StreamedCacheConnection::StreamedCacheConnection(StreamedCacheConnection&& other) :
    m_stream{ std::move(other.m_stream) },
    m_cache{ std::move(other.m_cache) }
{

}

StreamedCacheConnection::~StreamedCacheConnection()
{
    if (*this)
    {
        m_cache->access()->finalize();
        m_stream->close();
    }
}

StreamedCacheConnection& StreamedCacheConnection::operator=(StreamedCacheConnection&& other)
{
    if (this == &other) return *this;

    m_stream = std::move(other.m_stream);
    m_cache = std::move(other.m_cache);

    return *this;
}

StreamedCacheConnection::operator bool() const
{
    return m_stream && (*m_stream)
        && m_cache && (*m_cache->access());
}

CombinedCache& StreamedCacheConnection::cache()
{
    return *m_cache;
}

CombinedCache const& StreamedCacheConnection::cache() const
{
    return *m_cache;
}
