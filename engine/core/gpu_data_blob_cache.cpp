#include <algorithm>
#include <cassert>
#include <filesystem>
#include <utility>

#include "engine/core/global_settings.h"
#include "gpu_data_blob_cache.h"

namespace lexgine::core
{

GpuDataBlobCache::GpuDataBlobCache(GlobalSettings const& settings,
	size_t max_count/* = std::numeric_limits<size_t>::max()*/,
	bool is_read_only/* = false*/,
	bool allow_overwrites/* = true*/)
	: m_max_element_count{ max_count }
	, m_stream{ nullptr }
	, m_streamed_cache{ nullptr }
{
	if (!settings.isCacheEnabled())
	{
		return;
	}

	// create stream for the cache
	std::filesystem::path path_to_cache = settings.getCacheDirectory() / settings.getCacheName();
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

		m_stream.reset(new std::fstream{ path_to_cache.string(), cache_stream_mode });
	}

	// create cache instance
	if (m_stream && *m_stream)
	{
		if (does_cache_exist)
		{
			m_streamed_cache.reset(new GpuDataBlobStreamedCache{ *m_stream, is_read_only });

			if (!(*m_streamed_cache->access()))
			{
				// existing cache cannot be opened, probably due to data corruption
				// try to create new cache storage, if requested opening mode is not read-only
				if (is_read_only) { m_stream.release(); return; }
				m_stream->close();
				m_stream->open(path_to_cache.string(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
				if (!(*m_stream)) { m_stream.release(); return; }

				m_streamed_cache.reset(new GpuDataBlobStreamedCache{ *m_stream, settings.getMaxCombinedCacheSize(),
					global_constants::combined_cache_compression_level, allow_overwrites });
			}
		}
		else
		{
			m_streamed_cache.reset(new GpuDataBlobStreamedCache{ *m_stream, settings.getMaxCombinedCacheSize(),
				global_constants::combined_cache_compression_level, allow_overwrites });
		}
	}
}


GpuDataBlobCache::~GpuDataBlobCache()
{
	if (*this)
	{
		m_streamed_cache->access()->finalize();
		m_stream->close();
	}
}

GpuDataBlobCache::operator bool() const
{
	return m_stream && (*m_stream)
		&& m_streamed_cache && (*m_streamed_cache->access());
}

void GpuDataBlobCache::popOldest(size_t count)
{
    if (!m_max_element_count || !count)
        return;
    std::lock_guard lock { m_lock };
	count = (std::min)(count, m_priority_list.size());
	for (size_t i = 0; i < count; ++i)
	{
		EntryRecord& back = m_priority_list.back();
		m_in_memory_cache.erase(*back.key);
		m_priority_list.pop_back();
	}
	assert(m_priority_list.size() == m_in_memory_cache.size());
}

size_t GpuDataBlobCache::currentInMemoryCount() const
{
    assert(m_priority_list.size() == m_in_memory_cache.size());
    return m_priority_list.size();
}

size_t GpuDataBlobCache::currentOnDiskCount() const
{
    if (*this) {
        return m_streamed_cache->access()->getIndex().getNumberOfEntries();
    }
    return 0;
}

SharedDataChunk GpuDataBlobCache::find(GpuDataBlobCacheKey const& key) const
{
    if (m_max_element_count) 
	{
        std::lock_guard lock { m_lock };
        auto it = m_in_memory_cache.find(key);
        if (it != m_in_memory_cache.end()) 
		{
            m_priority_list.splice(m_priority_list.begin(), m_priority_list, it->second);
            return it->second->data;
        }
    }

	if (!*this) return {};
	auto disk_access = m_streamed_cache->access();
	if (!disk_access->doesEntryExist(key)) return {};
	SharedDataChunk materialized = disk_access->retrieveEntry(key);
	if (!materialized.data()) return {};

	if (m_max_element_count) 
	{
        std::lock_guard lock { m_lock };
        m_priority_list.push_front(EntryRecord { .data = materialized, .key = nullptr });
        auto new_it = m_priority_list.begin();
        auto [imc_it, ok] = m_in_memory_cache.emplace(key, new_it);
        (void)ok;
        assert(ok);
        new_it->key = &imc_it->first;
        if (m_priority_list.size() > m_max_element_count)
            const_cast<GpuDataBlobCache*>(this)->popOldest(m_priority_list.size() - m_max_element_count);
    }
	return materialized;
}

SharedDataChunk GpuDataBlobCache::find(GpuDataBlobCacheKey const& key, misc::DateTime const& min_timestamp) const
{
	if (!*this) return {};
	auto disk_access = m_streamed_cache->access();
	if (!disk_access->doesEntryExist(key) || disk_access->getEntryTimestamp(key) < min_timestamp) return {};
	return find(key);
}

void GpuDataBlobCache::put(GpuDataBlobCacheKey const& key, SharedDataChunk const& data)
{
	if (*this)
	{
		auto disk_access = m_streamed_cache->access();
		disk_access->addEntry(GpuDataBlobStreamedCache::entry_type{ key, data }, /*force_overwrite=*/true);
	}

	if (m_max_element_count)
	{
        std::lock_guard lock { m_lock };
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
	}
    if (m_priority_list.size() > m_max_element_count)
        popOldest(m_priority_list.size() - m_max_element_count);
}

}
