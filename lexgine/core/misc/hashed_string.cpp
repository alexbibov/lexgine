#include "hashed_string.h"
#include "strict_weak_ordering.h"

using namespace lexgine::core::misc;



thread_local spooky_hash_v2::SpookyHash HashedString::m_hash_generator{};


HashedString::HashedString() : 
    m_string{ "" }
{
    if (!m_hash_generator.IsInitialized())
        m_hash_generator.Init(m_seed1, m_seed2);

    m_hash = m_hash_generator.Hash64(m_string.c_str(), m_string.length(), m_seed3);
}

HashedString::HashedString(std::string const& str):
    m_string{ str }
{
    if (!m_hash_generator.IsInitialized())
        m_hash_generator.Init(m_seed1, m_seed2);

    m_hash = m_hash_generator.Hash64(m_string.c_str(), m_string.length(), m_seed3);
}

bool HashedString::operator<(HashedString const& other) const
{
    SWO_STEP(m_hash, < , other.m_hash);
    SWO_END(m_string, < , other.m_string);
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
