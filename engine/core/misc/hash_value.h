#ifndef LEXGINE_CORE_MISC_HASH_VALUE_H

#include <string>
#include <vector>
#include <compare>

#include "3rd_party/SpookyHash/SpookyV2.h"

namespace lexgine::core::misc {

class HashValue
{
public:
    HashValue(std::vector<uint8_t> const& data);
    HashValue(void const* p_data, size_t data_size);

    void combine(std::vector<uint8_t> const& data);
    void combine(void const* p_data, size_t data_size);

    auto operator<=>(HashValue const&) const = default;
    bool operator==(HashValue const&) const = default;

    uint64_t part1() const { return m_part1; }
    uint64_t part2() const { return m_part2; }

private:
    // the numbers for the seed values are taken from www.random.org
    static uint64_t const m_seed1 = 0x744dd29e659fecd9;
    static uint64_t const m_seed2 = 0x7f3a28db614e26d6;
    static thread_local spooky_hash_v2::SpookyHash m_hash_generator;

private:
    uint64_t m_part1;
    uint64_t m_part2;
};

}

#define LEXGINE_CORE_MISC_HASH_VALUE_H
#endif
