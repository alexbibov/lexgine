#include "fnv1a_64.h"

namespace lexgine::core::misc::hashes
{

void Fnv1a_64::create(void const* p_data, size_t data_size)
{
    if (!m_initialized)
    {
        m_hash_data.value = m_offset_basis;
        m_initialized = true;
        m_finalized = false;
        combine(p_data, data_size);
    }
}

void Fnv1a_64::combine(void const* p_data, size_t data_size)
{
    if (m_initialized && !m_finalized) 
    {
        auto const* p = static_cast<uint8_t const*>(p_data);
        uint64_t h = m_hash_data.value;
        for (size_t i = 0; i < data_size; ++i)
        {
            h ^= static_cast<uint64_t>(p[i]);
            h *= m_prime;
        }
        m_hash_data.value = h;
    }
}

std::strong_ordering Fnv1a_64::operator<=>(Fnv1a_64 const& other) const
{
    return m_hash_data.value <=> other.m_hash_data.value;
}

bool Fnv1a_64::operator==(Fnv1a_64 const& other) const
{
    return m_hash_data.value == other.m_hash_data.value;
}

uint64_t Fnv1a_64::fold() const
{
    return m_hash_data.value;
}

}
