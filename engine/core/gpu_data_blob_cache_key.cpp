#include <cassert>

#include "engine/core/misc/hashes/fnv1a_64.h"
#include "gpu_data_blob_cache_key.h"
namespace lexgine::core
{

std::string GpuDataBlobCacheKey::toString() const
{
	static constexpr char hex[] = "0123456789abcdef";
	std::string out;
	out.reserve(64);
	for (std::uint8_t byte : m_data.bytes) 
	{
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
	return static_cast<size_t>(fnv1a_64_hash.fold());
}

void GpuDataBlobCacheKey::serialize(void* p_serialization_blob) const
{
    if (m_used_bytes) 
	{
        memcpy(p_serialization_blob, m_data.bytes, serialized_size);
    }
	else
	{
        memset(p_serialization_blob, 0, serialized_size);
	}
}

void GpuDataBlobCacheKey::deserialize(void const* p_serialization_blob)
{
    memcpy(m_data.bytes, p_serialization_blob, serialized_size);
    CommonManifest* manifest = reinterpret_cast<CommonManifest*>(m_data.bytes);
    m_used_bytes = manifest->custom_segment_size + sizeof(CommonManifest);
    assert(m_used_bytes <= serialized_size);
    m_used_words = calculateUsedWords(m_used_bytes);
    assert(m_used_words >= 1);
}

bool GpuDataBlobCacheKey::operator<(GpuDataBlobCacheKey const& other) const
{
    // continue from here
    if (m_used_words < other.m_used_words)
        return true;
    if (m_used_words > other.m_used_words)
        return false;
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
    if (m_used_words != other.m_used_words)
        return false;
	for (size_t i = 0; i < m_used_words; ++i)
	{
		if (m_data.words[i] != other.m_data.words[i])
			return false;
	}
    return true;
}

GpuDataBlobCacheKey::CommonManifest GpuDataBlobCacheKey::createManifest(
    std::array<char, 4> const& magic_bytes,
    misc::UUID const& gpu_driver_uuid,
    misc::HashValue const& hash_value
)
{
    CommonManifest rv {};
	{
        union 
		{
            char b[4];
            uint32_t i;
        } helper;
        memcpy(helper.b, magic_bytes.data(), sizeof(magic_bytes));
        rv.magic = helper.i;
    }
    rv.gpu_driver_uuid = gpu_driver_uuid;
    size_t hash_width = hash_value.hashWidth();
    assert(hash_width <= sizeof(rv.hash));
    std::fill(rv.hash.begin(), rv.hash.end(), 0);
    memcpy(rv.hash.data(), hash_value.hashValue(), hash_width);
    return rv;
}

size_t GpuDataBlobCacheKey::calculateUsedWords(size_t used_bytes)
{
    return ((used_bytes + 7) & (~7)) / 8;
}

}
