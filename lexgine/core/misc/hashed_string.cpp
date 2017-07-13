#include "hashed_string.h"

using namespace lexgine::core::misc;



HashedString::HashedString(spooky_hash_v2::SpookyHash& hash_generator, uint64_t seed, std::string const& str) :
    m_string{ str },
    m_hash{ hash_generator.Hash64(str.c_str(), str.length(), seed) }
{

}

bool HashedString::operator<(HashedString const& other) const
{
    if (m_hash == other.m_hash)
        return m_string < other.m_string;

    return m_hash < other.m_hash;
}

bool HashedString::operator==(HashedString const& other) const
{
    return m_hash == other.m_hash && m_string == other.m_string;
}

uint64_t HashedString::hash() const
{
    return m_hash;
}

char const* HashedString::string() const
{
    return m_string.c_str();
}
