#include <cassert>

#include "engine/core/misc/hashes/xxhash64.h"
#include "engine/core/misc/hashes/xxhash128.h"

#include "hashed_string.h"
#include "hash_value.h"
#include "strict_weak_ordering.h"

namespace lexgine::core::misc {

HashedString::HashedString()
    : HashedString{""}
{

}

HashedString::HashedString(std::string const& str) 
    : m_string{ str }
{
    initHashes();
}

HashedString::HashedString(char const* p_chars)
    : m_string{ p_chars }
{
    initHashes();
}

bool HashedString::operator<(HashedString const& other) const
{
    SWO_STEP(m_short_hash, < , other.m_short_hash);
    SWO_STEP(m_long_hash, < , other.m_long_hash);
    SWO_END(m_string, < , other.m_string);
}

bool HashedString::operator==(HashedString const& other) const
{
    return m_string == other.m_string;
}

char const* HashedString::string() const
{
    return m_string.c_str();
}

void HashedString::initHashes()
{
    {
        hashes::XXHash128 lh{ m_string.data(), m_string.size() };
        auto v128 = lh.hashValueT();
        m_long_hash[0] = v128[0];
        m_long_hash[1] = v128[1];
    }
    {
        hashes::XXHash64 sh{ m_string.data(), m_string.size() };
        m_short_hash = sh.hashValueT();
    }
}

}