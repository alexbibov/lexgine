#include "hash_value.h"

namespace lexgine::core::misc {

thread_local spooky_hash_v2::SpookyHash HashValue ::m_hash_generator{};


 HashValue::HashValue(std::vector<uint8_t> const& data)
     : m_part1{ m_seed1 }
     , m_part2{ m_seed2 }
{
    m_hash_generator.Hash128(data.data(), data.size(), &m_part1, &m_part2);
}

 HashValue::HashValue(void const* p_data, size_t data_size)
    : m_part1 { m_seed1 }
    , m_part2 { m_seed2 }
{
    m_hash_generator.Hash128(p_data, data_size, &m_part1, &m_part2);
}

void HashValue::combine(std::vector<uint8_t> const& data)
{
    m_hash_generator.Hash128(data.data(), data.size(), &m_part1, &m_part2);
}

void HashValue::combine(void const* p_data, size_t data_size)
{
    m_hash_generator.Hash128(p_data, data_size, &m_part1, &m_part2);
}

}