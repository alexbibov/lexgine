#include "fnv1a_64.h"

namespace lexgine::core::misc::hashes
{

void Fnv1a_64::create(void const* p_data, size_t data_size)
{
    m_hashData.value = m_offset_basis;
    combine(p_data, data_size);
}

void Fnv1a_64::combine(void const* p_data, size_t data_size)
{
    auto const* p = static_cast<uint8_t const*>(p_data);
    uint64_t h = m_hashData.value;
    for (size_t i = 0; i < data_size; ++i)
    {
        h ^= static_cast<uint64_t>(p[i]);
        h *= m_prime;
    }
    m_hashData.value = h;
}

std::strong_ordering Fnv1a_64::operator<=>(Fnv1a_64 const& other) const
{
    return m_hashData.value <=> other.m_hashData.value;
}

bool Fnv1a_64::operator==(Fnv1a_64 const& other) const
{
    return m_hashData.value == other.m_hashData.value;
}

}
