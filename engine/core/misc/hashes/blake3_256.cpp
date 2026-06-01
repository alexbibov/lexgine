#include "blake3_256.h"

namespace lexgine::core::misc::hashes
{
void Blake3_256::create(void const* p_data, size_t data_size)
{
    if (!m_initialized)
    {
        blake3_hasher_init(&m_blake3_hasher);
        blake3_hasher_update(&m_blake3_hasher, p_data, data_size);
        m_initialized = true;
        m_finalized = false;
    }
}

void Blake3_256::combine(void const* p_data, size_t data_size)
{
    if (m_initialized && !m_finalized)
    {
        blake3_hasher_update(&m_blake3_hasher, p_data, data_size);
    }
}


void Blake3_256::finalize()
{
    if (m_initialized && !m_finalized)
    {
        blake3_hasher_finalize(&m_blake3_hasher, m_hash_data.data(), sizeof(m_hash_data));
        m_finalized = true;
        m_initialized = false;
    }
}

std::strong_ordering Blake3_256::operator<=>(Blake3_256 const& other) const
{
    return m_hash_data <=> other.m_hash_data;
}

bool Blake3_256::operator==(Blake3_256 const& other) const
{
    return m_hash_data == other.m_hash_data;
}

uint64_t Blake3_256::fold() const
{
    return foldBytes(m_hash_data.data(), m_hash_data.size());
}
}
