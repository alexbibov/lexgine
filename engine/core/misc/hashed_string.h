#ifndef LEXGINE_CORE_MISC_HASHED_STRING_H
#define LEXGINE_CORE_MISC_HASHED_STRING_H

#include <string>

#include "3rd_party/SpookyHash/SpookyV2.h"

namespace lexgine::core::misc {

class HashedString
{
private:

    // the numbers for the seed values are taken from www.random.org
    static uint64_t const m_seed1 = 0x744dd29e659fecd9;
    static uint64_t const m_seed2 = 0x7f3a28db614e26d6;
    static uint64_t const m_seed3 = 0xdf061c87b8fb0f8b;
    static thread_local spooky_hash_v2::SpookyHash m_hash_generator;


private:
    std::string m_string;
    uint64_t m_hash;

public:
    HashedString();
    HashedString(std::string const& str);

    bool operator<(HashedString const& other) const;
    bool operator==(HashedString const& other) const;

    uint64_t hash() const;
    char const* string() const;
};

}


// needed for simpler functioning of std::unordered_set and std::unordered_map
namespace std {
template<>
struct hash<lexgine::core::misc::HashedString>
{
    using argument_type = lexgine::core::misc::HashedString;
    using result_type = size_t;

    result_type operator()(argument_type const& key) const
    {
        return key.hash();
    }
};

}

#endif
