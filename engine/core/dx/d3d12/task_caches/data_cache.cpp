#include <fstream>
#include <utility>

#include "data_cache.h"
#include "engine/core/globals.h"
#include "engine/core/global_settings.h"
#include "engine/core/exception.h"
#include "engine/core/misc/misc.h"


using namespace lexgine::core;
using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12::task_caches;


DataCache::DataCache(GlobalSettings const& global_settings, bool is_read_only, bool allow_overwrites/* = true*/) :
    m_stream{ nullptr },
    m_cache{ nullptr }
{
    if (!global_settings.isCacheEnabled())
    {
        return;
    }

    // create stream for the cache
    std::filesystem::path path_to_cache = global_settings.getCacheDirectory() / global_settings.getCacheName();
    bool does_cache_exist{ false };
    {
        auto cache_stream_mode = std::ios_base::in | std::ios_base::binary;
        if (!is_read_only) cache_stream_mode |= std::ios_base::out;

        does_cache_exist = std::filesystem::exists(path_to_cache);
        if (!does_cache_exist)
        {
            if (is_read_only) return;
            cache_stream_mode |= std::ios_base::trunc;
        }

        m_stream.reset(new std::fstream{ path_to_cache.string(), cache_stream_mode});
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
                m_stream->open(path_to_cache.string(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
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

DataCache::DataCache(DataCache&& other) :
    m_stream{ std::move(other.m_stream) },
    m_cache{ std::move(other.m_cache) }
{

}

DataCache::~DataCache()
{
    if (*this)
    {
        m_cache->access()->finalize();
        m_stream->close();
    }
}

DataCache& DataCache::operator=(DataCache&& other)
{
    if (this == &other) return *this;

    m_stream = std::move(other.m_stream);
    m_cache = std::move(other.m_cache);

    return *this;
}

DataCache::operator bool() const
{
    return m_stream && (*m_stream)
        && m_cache && (*m_cache->access());
}

CombinedCache& DataCache::cache()
{
    return *m_cache;
}

CombinedCache const& DataCache::cache() const
{
    return *m_cache;
}
