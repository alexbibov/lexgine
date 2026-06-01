#include "xxhash64.h"

namespace lexgine::core::misc::hashes {

XXHash64::XXHash64(void const* p_data, size_t data_size)
{
    XXH64_hash_t hash = XXH3_64bits(p_data, data_size);
    m_hash_data.value = static_cast<uint64_t>(hash);
    m_finalized = true;
}

void XXHash64::create(void const* p_data, size_t data_size)
{
    if (!m_initialized)
    {
        m_xxh3_state = XXH3_createState();
        XXH3_64bits_reset(m_xxh3_state);
        XXH3_64bits_update(m_xxh3_state, &data_size, sizeof(data_size));
        XXH3_64bits_update(m_xxh3_state, p_data, data_size);
        m_initialized = true;
        m_finalized = false;
    }
}

void XXHash64::combine(void const* p_data, size_t data_size)
{
    if (m_initialized && !m_finalized)
    {
        XXH3_64bits_update(m_xxh3_state, &data_size, sizeof(data_size));
        XXH3_64bits_update(m_xxh3_state, p_data, data_size);
    }
}

void XXHash64::finalize()
{
    if (m_initialized && !m_finalized)
    {
        m_hash_data.value = static_cast<uint64_t>(XXH3_64bits_digest(m_xxh3_state));
        XXH3_freeState(m_xxh3_state);
        m_xxh3_state = nullptr;
        m_finalized = true;
        m_initialized = false;
    }
}

std::strong_ordering XXHash64::operator<=>(XXHash64 const& other) const
{
    return m_hash_data.value <=> other.m_hash_data.value;
}

bool XXHash64::operator==(XXHash64 const& other) const
{
    return m_hash_data.value == other.m_hash_data.value;
}

uint64_t XXHash64::fold() const
{
    return m_hash_data.value;
}

}
