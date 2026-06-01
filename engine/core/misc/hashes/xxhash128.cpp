
#include <cstring>
#include "xxhash128.h"

namespace lexgine::core::misc::hashes {

XXHash128::XXHash128(void const* p_data, size_t data_size)
{
    XXH128_hash_t hash = XXH3_128bits(p_data, data_size);
    m_hash_data.words[0] = static_cast<uint64_t>(hash.low64);
    m_hash_data.words[1] = static_cast<uint64_t>(hash.high64);
    m_finalized = true;
}

void XXHash128::create(void const* p_data, size_t data_size)
{
    if (!m_initialized)
    {
        m_xxh3_state = XXH3_createState();
        XXH3_128bits_reset(m_xxh3_state);
        XXH3_128bits_update(m_xxh3_state, &data_size, sizeof(data_size));
        XXH3_128bits_update(m_xxh3_state, p_data, data_size);
        m_initialized = true;
        m_finalized = false;
    }
}

void XXHash128::combine(void const* p_data, size_t data_size)
{
    if (m_initialized && !m_finalized)
    {
        XXH3_128bits_update(m_xxh3_state, &data_size, sizeof(data_size));
        XXH3_128bits_update(m_xxh3_state, p_data, data_size);
    }
}

void XXHash128::finalize()
{
    if (m_initialized && !m_finalized)
    {
        XXH128_hash_t hash = XXH3_128bits_digest(m_xxh3_state);
        m_hash_data.words[0] = static_cast<uint64_t>(hash.low64);
        m_hash_data.words[1] = static_cast<uint64_t>(hash.high64);
        XXH3_freeState(m_xxh3_state);
        m_xxh3_state = nullptr;
        m_finalized = true;
        m_initialized = false;
    }
}

std::strong_ordering XXHash128::operator<=>(XXHash128 const& other) const
{
    int cmp_result = std::memcmp(m_hash_data.bytes, other.m_hash_data.bytes, sizeof(m_hash_data));
    if (cmp_result < 0)
        return std::strong_ordering::less;
    if (cmp_result > 0)
        return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

bool XXHash128::operator==(XXHash128 const& other) const
{
    int cmp_result = std::memcmp(m_hash_data.bytes, other.m_hash_data.bytes, sizeof(m_hash_data));
    return cmp_result == 0;
}

uint64_t XXHash128::fold() const
{
    return m_hash_data.words[0] ^ m_hash_data.words[1];
}

}
