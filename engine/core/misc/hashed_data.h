#ifndef LEXGINE_CORE_MISC_HASHED_DATA_H
#define LEXGINE_CORE_MISC_HASHED_DATA_H

#include <string>
#include <vector>

#include "3rd_party/SpookyHash/SpookyV2.h"

namespace lexgine::core::misc {

struct HashValue
{
    uint64_t part1;
    uint64_t part2;
};

class HashedData
{
private:

    // the numbers for the seed values are taken from www.random.org
    static uint64_t const m_seed1 = 0x744dd29e659fecd9;
    static uint64_t const m_seed2 = 0x7f3a28db614e26d6;
    static uint64_t const m_seed3 = 0xdf061c87b8fb0f8b;
    static thread_local spooky_hash_v2::SpookyHash m_hash_generator;

private:
    HashValue m_hash;

public:
    HashedData();
    HashedData(std::vector<uint8_t> const& data);
    HashedData(void const* data, size_t data_length);


};

}

#endif
