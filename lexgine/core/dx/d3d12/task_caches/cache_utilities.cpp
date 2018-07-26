#include "cache_utilities.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/misc/misc.h"

#include <list>
#include <fstream>


using namespace lexgine::core;
using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12::task_caches;

namespace {

misc::Optional<std::fstream> createCacheStream(std::string const& path_to_cache, bool is_read_only)
{
    auto cache_stream_mode = std::ios_base::in | std::ios_base::binary;
    if (!is_read_only) cache_stream_mode |= std::ios_base::out;

    bool does_cache_exist = doesFileExist(path_to_cache);
    if (!does_cache_exist)
    {
        if (is_read_only) return misc::Optional<std::fstream>{};
        cache_stream_mode |= std::ios_base::trunc;
    }

    return misc::Optional<std::fstream>{std::fstream{ path_to_cache, cache_stream_mode }};
}

misc::Optional<CombinedCache> createCombinedCache(GlobalSettings const& global_settings,
    std::string const& path_to_cache, misc::Optional<std::fstream>& cache_stream,
    bool is_read_only, bool allow_overwrites)
{
    if (!cache_stream.isValid()) return misc::Optional<CombinedCache>{};

    CombinedCache cache{ static_cast<std::fstream&>(cache_stream), is_read_only };

    if (!cache)
    {
        // existing cache cannot be opened, probably due to data corruption
        // try to create new cache storage, if requested opening mode is not read-only
        if (is_read_only) return misc::Optional<CombinedCache>{};

        std::fstream& s = static_cast<std::fstream&>(cache_stream);
        s.close();
        s.open(path_to_cache, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
        if (!s) return misc::Optional<CombinedCache>{};

        CombinedCache cache{ s, global_settings.getMaxCombinedCacheSize(), global_constants::combined_cache_compression_level, allow_overwrites };
        if (!cache) return misc::Optional<CombinedCache>{};

        return misc::Optional<CombinedCache>{cache};
    }

    return misc::Optional<CombinedCache>{cache};
}

}


StreamedCacheConnection lexgine::core::dx::d3d12::task_caches::establishConnectionWithCombinedCache(GlobalSettings const& global_settings, uint8_t worker_id, bool readonly_mode, bool allow_overwrites)
{
    std::string path_to_cache = global_settings.getCacheDirectory() + global_settings.getCombinedCacheName()
        + "." + global_constants::combined_cache_extra_extension + std::to_string(worker_id);

    return establishConnectionWithCombinedCache(global_settings, path_to_cache, readonly_mode, allow_overwrites);
}


StreamedCacheConnection lexgine::core::dx::d3d12::task_caches::establishConnectionWithCombinedCache(GlobalSettings const& global_settings, std::string const& path_to_combined_cache, bool readonly_mode, bool allow_overwrites /* = true */)
{
    return StreamedCacheConnection{ global_settings, path_to_combined_cache, readonly_mode, allow_overwrites };


#if 0
    auto cache_stream_mode = std::ios_base::in | std::ios_base::binary;
    if (!readonly_mode) cache_stream_mode |= std::ios_base::out;

    bool cache_exists = doesFileExist(path_to_combined_cache);
    if (!cache_exists)
    {
        if (readonly_mode) return Optional<StreamedCacheConnection>{};
        cache_stream_mode |= std::ios_base::trunc;
    }

    std::unique_ptr<std::fstream> cache_stream = std::make_unique<std::fstream>(path_to_combined_cache, cache_stream_mode);
    if (!cache_stream->good()) return Optional<StreamedCacheConnection>{};

    if (cache_exists)
    {
        CombinedCache cache{ *cache_stream, readonly_mode };
        if (!cache)
        {
            // existing cache cannot be opened, probably due to data corruption
            // try to create new cache storage, if requested opening mode is not read-only
            if (readonly_mode) return Optional<StreamedCacheConnection>{};
            cache_stream_mode |= std::ios_base::trunc;

            cache_stream->open(path_to_combined_cache, cache_stream_mode);
            if (!cache_stream->good()) return Optional<StreamedCacheConnection>{};

            CombinedCache cache{ *cache_stream, global_settings.getMaxCombinedCacheSize(),
            global_constants::combined_cache_compression_level, allow_overwrites };
            if (!cache) return Optional<StreamedCacheConnection>{};

            return Optional<StreamedCacheConnection>{StreamedCacheConnection{ std::move(cache), std::move(cache_stream) }};
        }

        return Optional<StreamedCacheConnection>{StreamedCacheConnection{ std::move(cache), std::move(cache_stream) }};
    }
    else
    {
        CombinedCache cache{ *cache_stream, global_settings.getMaxCombinedCacheSize(),
        global_constants::combined_cache_compression_level, allow_overwrites };
        if(!cache) return Optional<StreamedCacheConnection>{};

        return Optional<StreamedCacheConnection>{StreamedCacheConnection{ std::move(cache), std::move(cache_stream) }};
    }

#endif
}


misc::Optional<StreamedCacheConnection> lexgine::core::dx::d3d12::task_caches::findCombinedCacheContainingKey(CombinedCacheKey const& key, GlobalSettings const& global_settings)
{
    auto combined_caches = getFilesInDirectory(global_settings.getCacheDirectory(),
        global_settings.getCombinedCacheName() + "." + global_constants::combined_cache_extra_extension + "*");

    for (auto const& path_to_cache : combined_caches)
    {
        auto cache_connection = establishConnectionWithCombinedCache(global_settings, path_to_cache, true);
        if (cache_connection)
        {
            if (cache_connection.cache().doesEntryExist(key))
                return misc::Optional<StreamedCacheConnection>{std::move(cache_connection)};
        }
    }

    return misc::Optional<StreamedCacheConnection>{};
}



StreamedCacheConnection::StreamedCacheConnection(GlobalSettings const& global_settings, std::string const& path_to_cache, 
    bool is_read_only, bool allow_overwrites/* = true*/):
    m_stream{ createCacheStream(path_to_cache, is_read_only) },
    m_cache{ createCombinedCache(global_settings, path_to_cache, m_stream, is_read_only, allow_overwrites) }
{
}

StreamedCacheConnection::StreamedCacheConnection(StreamedCacheConnection&& other):
    m_stream{ std::move(other.m_stream) },
    m_cache{ std::move(other.m_cache) }
{
}

StreamedCacheConnection::~StreamedCacheConnection()
{
    if (m_cache.isValid()) static_cast<CombinedCache&>(m_cache).finalize();
    if (m_stream.isValid()) static_cast<std::fstream&>(m_stream).close();
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
    return m_stream.isValid() && m_cache.isValid();
}

CombinedCache& StreamedCacheConnection::cache()
{
    static_cast<CombinedCache&>(m_cache);
}
