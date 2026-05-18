#ifndef LEXGINE_CORE_GPU_DATA_BLOB_CACHE_H
#define LEXGINE_CORE_GPU_DATA_BLOB_CACHE_H

#include <fstream>
#include <list>
#include <limits>
#include <memory>
#include <unordered_map>
#include <mutex>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/data_blob.h"
#include "engine/core/global_constants.h"
#include "engine/core/gpu_data_blob_cache_key.h"
#include "engine/core/misc/datetime.h"
#include "engine/core/streamed_cache.h"

namespace lexgine::core
{

using GpuDataBlobStreamedCache = StreamedCacheConcurrencySentinel<
	GpuDataBlobCacheKey,
	global_constants::combined_cache_cluster_size>;

class GpuDataBlobCache final
{
public:
	GpuDataBlobCache(GlobalSettings const& settings,
		size_t max_count = (std::numeric_limits<size_t>::max)(),
		bool is_read_only = false,
		bool allow_overwrites = true);
    GpuDataBlobCache(GpuDataBlobCache const&) = delete;
    GpuDataBlobCache(GpuDataBlobCache&&) = delete;
	~GpuDataBlobCache();

	GpuDataBlobCache& operator=(GpuDataBlobCache const&) = delete;
	GpuDataBlobCache& operator=(GpuDataBlobCache&&) = delete;

	explicit operator bool() const;

	void popOldest(size_t count);
	size_t currentInMemoryCount() const;
	size_t currentOnDiskCount() const;
	SharedDataChunk find(GpuDataBlobCacheKey const& key) const;
	SharedDataChunk find(GpuDataBlobCacheKey const& key, misc::DateTime const& min_timestamp) const;
	void put(GpuDataBlobCacheKey const& key, SharedDataChunk const& data);

private:
	struct EntryRecord
	{
		SharedDataChunk data;
		GpuDataBlobCacheKey const* key;
	};

private:
	size_t m_max_element_count;
    mutable std::mutex m_lock;
	mutable std::list<EntryRecord> m_priority_list;
	mutable std::unordered_map<
		GpuDataBlobCacheKey,
		std::list<EntryRecord>::iterator,
		GpuDataBlobCacheKeyHasher
	> m_in_memory_cache;
	std::unique_ptr<std::fstream> m_stream;
	std::unique_ptr<GpuDataBlobStreamedCache> m_streamed_cache;
};

}

#endif
