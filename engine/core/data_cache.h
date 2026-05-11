#ifndef LEXGINE_CORE_DATA_CACHE_H
#define LEXGINE_CORE_DATA_CACHE_H

#include <fstream>
#include <memory>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/global_constants.h"
#include "engine/core/streamed_cache.h"
#include "engine/core/misc/optional.h"

#include "engine/core/dx/d3d12/tasks/root_signature_builder.h"
#include "engine/core/dx/d3d12/tasks/pso_compilation_task.h"
#include "engine/core/dx/d3d12/tasks/hlsl_compilation_task.h"
#include "engine/core/gpu_data_blob_cache_key.h"


namespace lexgine::core
{

using CombinedCache = StreamedCacheConcurrencySentinel<GpuDataBlobCacheKey, global_constants::combined_cache_cluster_size>;

class DataCache
{
public:
    DataCache(GlobalSettings const& global_settings, bool is_read_only, bool allow_overwrites = true);
    DataCache(DataCache&& other);
    ~DataCache();

    DataCache& operator=(DataCache&& other);

    operator bool() const;

    CombinedCache& cache();
    CombinedCache const& cache() const;

private:
    std::unique_ptr<std::fstream> m_stream;
    std::unique_ptr<CombinedCache> m_cache;
};

}

#endif
