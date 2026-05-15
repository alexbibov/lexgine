#ifndef LEXGINE_CORE_DX_D3D12_GPU_DATA_BLOB_CACHE_H
#define LEXGINE_CORE_DX_D3D12_GPU_DATA_BLOB_CACHE_H

#include <fstream>
#include <list>
#include <memory>
#include <unordered_map>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/global_constants.h"
#include "engine/core/gpu_data_blob_cache_key.h"
#include "engine/core/streamed_cache.h"
#include "engine/core/dx/d3d12/d3d_data_blob.h"

namespace lexgine::core::dx::d3d12
{

using GpuDataBlobStreamedCache = ::lexgine::core::StreamedCacheConcurrencySentinel<
	::lexgine::core::GpuDataBlobCacheKey,
	::lexgine::core::global_constants::combined_cache_cluster_size>;

class GpuDataBlobCache
{
public:
	GpuDataBlobCache(GlobalSettings const& settings,
		size_t max_count = (std::numeric_limits<size_t>::max)(),
		bool is_read_only = false,
		bool allow_overwrites = true);
	GpuDataBlobCache(GpuDataBlobCache&& other);
	~GpuDataBlobCache();

	GpuDataBlobCache& operator=(GpuDataBlobCache&& other);

	explicit operator bool() const;

	GpuDataBlobStreamedCache& streamedCache();
	GpuDataBlobStreamedCache const& streamedCache() const;

	void popOldest(size_t count);
	size_t currentCount() const;
	D3DDataBlob find(GpuDataBlobCacheKey const& key) const;
	void put(GpuDataBlobCacheKey const& key, D3DDataBlob const& data);

private:
	struct EntryRecord
	{
		D3DDataBlob data;
		GpuDataBlobCacheKey const* key;
	};

private:
	size_t m_max_element_count;
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
