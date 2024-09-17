#include <cassert>

#include "hashed_string.h"
#include "hash_value.h"
#include "strict_weak_ordering.h"

namespace lexgine::core::misc {


HashedString::HashedString()
    : HashedString{ "" }
{
}

HashedString::HashedString(std::string const& str) :
    m_string{ str }
{
    HashValue hash{ m_string.data(), m_string.length() };
    m_hash = hash.part1() ^ hash.part2();
}

bool HashedString::operator<(HashedString const& other) const
{
    SWO_END(m_hash, < , other.m_hash);
}

bool HashedString::operator==(HashedString const& other) const
{
#ifdef _DEBUG
    assert((m_hash != other.m_hash || m_string == other.m_string) && "Hash collision detected");
#endif

    return m_hash == other.m_hash;
}

uint64_t HashedString::hash() const
{
    return m_hash;
}

char const* HashedString::string() const
{
    return m_string.c_str();
}

}