#include <algorithm>
#include <cassert>
#include <cstring>

#include <d3dcompiler.h>

#include "engine/core/data_blob.h"
#include "engine/core/gpu_data_blob_on_disk_streamed_cache.h"
#include "gpu_data_blob_cache.h"

namespace lexgine::core::dx::d3d12
{

namespace {

D3DDataBlob materializeFromChunk(SharedDataChunk const& chunk)
{
	if (!chunk.size() || !chunk.data()) return D3DDataBlob{ nullptr };
	Microsoft::WRL::ComPtr<ID3DBlob> d3d_blob{ nullptr };
	HRESULT res = D3DCreateBlob(chunk.size(), d3d_blob.GetAddressOf());
	if (res != S_OK && res != S_FALSE) return D3DDataBlob{ nullptr };
	assert(d3d_blob->GetBufferSize() >= chunk.size());
	std::memcpy(d3d_blob->GetBufferPointer(), chunk.data(), chunk.size());
	return D3DDataBlob{ d3d_blob };
}

}  // namespace

GpuDataBlobCache::GpuDataBlobCache(Globals& globals, size_t max_count/* = std::numeric_limits<size_t>::max()*/)
	: m_streamed_on_disk_cache{ *globals.get<GpuDataBlobOnDiskStreamedCache>() }
	, m_max_element_count{ max_count }
{

}

void GpuDataBlobCache::popOldest(size_t count)
{
	count = (std::min)(count, m_priority_list.size());
	for (size_t i = 0; i < count; ++i)
	{
		EntryRecord& back = m_priority_list.back();
		m_in_memory_cache.erase(*back.key);
		m_priority_list.pop_back();
	}
	assert(m_priority_list.size() == m_in_memory_cache.size());
}

size_t GpuDataBlobCache::currentCount() const
{
	assert(m_priority_list.size() == m_in_memory_cache.size());
	return m_priority_list.size();
}

D3DDataBlob GpuDataBlobCache::find(GpuDataBlobCacheKey const& key) const
{
	auto it = m_in_memory_cache.find(key);
	if (it != m_in_memory_cache.end())
	{
		m_priority_list.splice(m_priority_list.begin(), m_priority_list, it->second);
		return it->second->data;
	}

	if (!m_streamed_on_disk_cache) return {};
	auto disk_access = m_streamed_on_disk_cache.cache().access();
	if (!disk_access->doesEntryExist(key)) return {};
	D3DDataBlob materialized = materializeFromChunk(disk_access->retrieveEntry(key));
	if (!materialized.native()) return {};

	m_priority_list.push_front(EntryRecord{ .data = materialized, .key = nullptr });
	auto new_it = m_priority_list.begin();
	auto [imc_it, ok] = m_in_memory_cache.emplace(key, new_it);
	(void)ok;
	assert(ok);
	new_it->key = &imc_it->first;
	if (m_priority_list.size() > m_max_element_count)
		const_cast<GpuDataBlobCache*>(this)->popOldest(m_priority_list.size() - m_max_element_count);
	return materialized;
}

void GpuDataBlobCache::put(GpuDataBlobCacheKey const& key, D3DDataBlob const& data)
{
	if (m_streamed_on_disk_cache)
	{
		auto disk_access = m_streamed_on_disk_cache.cache().access();
		disk_access->addEntry(GpuDataBlobStreamedCache::entry_type{ key, data }, /*force_overwrite=*/true);
	}

	auto it = m_in_memory_cache.find(key);
	if (it != m_in_memory_cache.end())
	{
		it->second->data = data;
		m_priority_list.splice(m_priority_list.begin(), m_priority_list, it->second);
		return;
	}

	m_priority_list.push_front(EntryRecord{ .data = data, .key = nullptr });
	auto new_element_it = m_priority_list.begin();
	auto [new_element_it_imc, result] = m_in_memory_cache.emplace(key, new_element_it);
	(void)result;
	assert(result);
	new_element_it->key = &new_element_it_imc->first;
	assert(m_in_memory_cache.size() == m_priority_list.size());
	if (m_priority_list.size() > m_max_element_count)
		popOldest(m_priority_list.size() - m_max_element_count);
}

}
