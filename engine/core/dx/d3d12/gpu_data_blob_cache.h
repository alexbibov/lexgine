#ifndef LEXGINE_CORE_DX_D3D12_GPU_DATA_BLOB_CACHE_H
#define LEXGINE_CORE_DX_D3D12_GPU_DATA_BLOB_CACHE_H

#include <list>
#include <unordered_map>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/globals.h"
#include "engine/core/gpu_data_blob_cache_key.h"
#include "engine/core/dx/d3d12/d3d_data_blob.h"

namespace lexgine::core::dx::d3d12
{

class GpuDataBlobCache
{
public:
	GpuDataBlobCache(Globals& globals, size_t max_count = (std::numeric_limits<size_t>::max)());
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
	GpuDataBlobOnDiskStreamedCache& m_streamed_on_disk_cache;
	size_t m_max_element_count;
	mutable std::list<EntryRecord> m_priority_list;
	mutable std::unordered_map<
		GpuDataBlobCacheKey,
		std::list<EntryRecord>::iterator,
		GpuDataBlobCacheKeyHasher
	> m_in_memory_cache;
};

}

#endif