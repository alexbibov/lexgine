#ifndef LEXGINE_CORE_DX_D3D12_TASK_CACHES_CACHE_UTILITIES_H
#define LEXGINE_CORE_DX_D3D12_TASK_CACHES_CACHE_UTILITIES_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/global_constants.h"
#include "lexgine/core/streamed_cache.h"
#include "lexgine/core/misc/optional.h"
#include "combined_cache_key.h"

#include <fstream>

namespace lexgine { namespace core { namespace dx { namespace d3d12 { namespace task_caches {

using CombinedCache = StreamedCache<CombinedCacheKey, global_constants::combined_cache_cluster_size>;

class StreamedCacheConnection
{
public:
    StreamedCacheConnection(GlobalSettings const& global_settings, std::string const& path_to_cache, 
        bool is_read_only, bool allow_overwrites = true);
    StreamedCacheConnection(StreamedCacheConnection&& other);
    ~StreamedCacheConnection();

    StreamedCacheConnection& operator=(StreamedCacheConnection&& other);

    operator bool() const;

    CombinedCache& cache();

private:
    misc::Optional<std::fstream> m_stream;
    misc::Optional<CombinedCache> m_cache;
};

/*! establishes connection with combined cache associated with the given worker id.
 In case if it is not possible to establish connection with requested cache returns invalid Optional<> object
*/
StreamedCacheConnection establishConnectionWithCombinedCache(GlobalSettings const& global_settings, uint8_t worker_id, bool readonly_mode, bool allow_overwrites = true);

/*! Establishes connection with combined cache located at the given path.
 In case if connection cannot be made, the return value will be an invalid Optional<> object
*/
StreamedCacheConnection establishConnectionWithCombinedCache(GlobalSettings const& global_settings, std::string const& path_to_combined_cache, bool readonly_mode, bool allow_overwrites = true);

/*! locates combined cache containing requested key and establishes connection with it.
 Returns invalid Optional<> object in case if connection cannot be established (the most common reason is 
 that the key does not exist in any of the combined caches)
*/
misc::Optional<StreamedCacheConnection> findCombinedCacheContainingKey(CombinedCacheKey const& key, GlobalSettings const& global_settings);

}}}}}

#endif
