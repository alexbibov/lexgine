#include <cassert>

#include "engine/core/misc/hashes/fnv1a_64.h"
#include "gpu_data_blob_cache_key.h"
#include <any>
namespace lexgine::core
{

std::string GpuDataBlobCacheKey::toString() const
{
	static constexpr char hex[] = "0123456789abcdef";

	std::string out;
	out.reserve(64);

	for (std::uint8_t byte : m_data.bytes) {
		out.push_back(hex[byte >> 4]);
		out.push_back(hex[byte & 0x0F]);
	}

	return "0x" + out;
}

size_t GpuDataBlobCacheKey::hash() const
{
	misc::hashes::Fnv1a_64 fnv1a_64_hash{};
	auto& full_hash_value = reinterpret_cast<CommonManifest const*>(&m_data)->hash;
	fnv1a_64_hash.create(&full_hash_value, sizeof(full_hash_value));
	fnv1a_64_hash.finalize();
	return static_cast<size_t>(fnv1a_64_hash.hashValueT());
}

void GpuDataBlobCacheKey::serialize(void* p_serialization_blob) const
{
    memcpy(p_serialization_blob, &m_data, m_used_bytes);
}

void GpuDataBlobCacheKey::deserialize(void const* p_serialization_blob)
{
    memcpy(&m_data, p_serialization_blob, m_used_bytes);
}

bool GpuDataBlobCacheKey::operator<(GpuDataBlobCacheKey const& other) const
{
	for (size_t i = 0; i < m_used_words; ++i)
	{
		if (m_data.words[i] < other.m_data.words[i])
			return true;
		if (m_data.words[i] > other.m_data.words[i])
			return false;
	}
	return false;
}

bool GpuDataBlobCacheKey::operator==(GpuDataBlobCacheKey const& other) const
{
	for (size_t i = 0; i < m_used_words; ++i)
	{
		if (m_data.words[i] != other.m_data.words[i])
			return false;
	}
    return true;
}

}