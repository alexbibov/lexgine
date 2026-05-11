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
	GpuDataBlobCache(Globals& globals);

private:
	std::list<D3DDataBlob> m_priority_list;
	std::unordered_map<GpuDataBlobCacheKey, D3DDataBlob> m_in_memory_cache;
	DataCache& m_streamed_on_disk_cache;
};

}

#endif