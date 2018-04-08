#include "cache_utilities.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/misc/misc.h"

#include <list>
#include <fstream>


using namespace lexgine::core;
using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12::task_caches;


Optional<StreamedCacheConnection> lexgine::core::dx::d3d12::task_caches::establishConnectionWithCombinedCache(GlobalSettings const& global_settings, uint8_t worker_id, bool readonly_mode, bool allow_overwrites)
{
    std::string path_to_cache = global_settings.getCacheDirectory() + global_settings.getCombinedCacheName()
        + "." + global_constants::combined_cache_extra_extension + std::to_string(worker_id);

    return establishConnectionWithCombinedCache(global_settings, path_to_cache, readonly_mode, allow_overwrites);
}


Optional<StreamedCacheConnection> lexgine::core::dx::d3d12::task_caches::establishConnectionWithCombinedCache(GlobalSettings const& global_settings, std::string const& path_to_combined_cache, bool readonly_mode, bool allow_overwrites /* = true */)
{
    auto cache_stream_mode = std::ios_base::in | std::ios_base::binary;
    if (!readonly_mode) cache_stream_mode |= std::ios_base::out;

    bool cache_exists = doesFileExist(path_to_combined_cache);
    if (!cache_exists)
    {
        if (readonly_mode) return Optional<StreamedCacheConnection>{};
        cache_stream_mode |= std::ios_base::trunc;
    }

    std::fstream cache_stream{ path_to_combined_cache, cache_stream_mode };
    if (!cache_stream) return Optional<StreamedCacheConnection>{};

    if (cache_exists)
    {
        CombinedCache cache{ cache_stream, readonly_mode };
        if (!cache)
        {
            // existing cache cannot be opened, probably due to data corruption
            // try to create new cache storage, if requested opening mode is not read-only
            if (readonly_mode) return Optional<StreamedCacheConnection>{};
            cache_stream_mode |= std::ios_base::trunc;

            cache_stream.open(path_to_combined_cache, cache_stream_mode);
            if (!cache_stream) return Optional<StreamedCacheConnection>{};

            CombinedCache cache{ cache_stream, global_settings.getMaxCombinedCacheSize(),
            global_constants::combined_cache_compression_level, allow_overwrites };
            if (!cache) return Optional<StreamedCacheConnection>{};

            return Optional<StreamedCacheConnection>{StreamedCacheConnection{ std::move(cache), std::move(cache_stream) }};
        }

        return Optional<StreamedCacheConnection>{StreamedCacheConnection{ std::move(cache), std::move(cache_stream) }};
    }
    else
    {
        CombinedCache cache{ cache_stream, global_settings.getMaxCombinedCacheSize(),
        global_constants::combined_cache_compression_level, allow_overwrites };
        if(!cache) return Optional<StreamedCacheConnection>{};

        return Optional<StreamedCacheConnection>{StreamedCacheConnection{ std::move(cache), std::move(cache_stream) }};
    }
}


Optional<StreamedCacheConnection> lexgine::core::dx::d3d12::task_caches::findCombinedCacheContainingKey(CombinedCacheKey const& key, GlobalSettings const& global_settings)
{
    auto combined_caches = getFilesInDirectory(global_settings.getCacheDirectory(),
        global_settings.getCombinedCacheName() + "." + global_constants::combined_cache_extra_extension + "*");

    for (auto const& path_to_cache : combined_caches)
    {
        auto cache_connection = establishConnectionWithCombinedCache(global_settings, path_to_cache, true);
        if (cache_connection.isValid())
        {
            if (static_cast<StreamedCacheConnection&>(cache_connection).cache.doesEntryExist(key))
                return Optional<StreamedCacheConnection>{StreamedCacheConnection{ std::move(static_cast<StreamedCacheConnection&>(cache_connection)) }};
        }
    }

    return Optional<StreamedCacheConnection>{};
}
