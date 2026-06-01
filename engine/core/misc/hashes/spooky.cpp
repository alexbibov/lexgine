#include "spooky.h"

namespace lexgine::core::misc::hashes {

thread_local spooky_hash_v2::SpookyHash SpookyHashValue::m_hash_generator{};

void SpookyHashValue::create(void const* p_data, size_t data_size)
{
    if (!m_initialized)
    {
        m_hash_data.words[0] = m_seed1;
        m_hash_data.words[1] = m_seed2;
        m_hash_generator.Hash128(p_data, data_size, &m_hash_data.words[0], &m_hash_data.words[1]);
        m_initialized = true;
        m_finalized = false;
    }
}

void SpookyHashValue::combine(void const* p_data, size_t data_size)
{
    if (m_initialized && !m_finalized)
    {
        m_hash_generator.Hash128(p_data, data_size, &m_hash_data.words[0], &m_hash_data.words[1]);
    }
}

std::strong_ordering SpookyHashValue::operator<=>(SpookyHashValue const& other) const
{
    int cmp_result = std::memcmp(m_hash_data.bytes, other.m_hash_data.bytes, sizeof(m_hash_data));
    if (cmp_result < 0)
        return std::strong_ordering::less;
    if (cmp_result > 0)
        return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

bool SpookyHashValue::operator==(SpookyHashValue const& other) const
{
    int cmp_result = std::memcmp(m_hash_data.bytes, other.m_hash_data.bytes, sizeof(m_hash_data));
    return cmp_result == 0;
}

uint64_t SpookyHashValue::fold() const
{
    return m_hash_data.words[0] ^ m_hash_data.words[1];
}

}
