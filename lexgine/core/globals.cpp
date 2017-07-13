#include "globals.h"
#include "../../../3rd_party/SpookyHash/SpookyV2.h"

#include <chrono>

using namespace lexgine::core;


Globals* Globals::m_p_self{ nullptr };



Globals::StringAndHash::StringAndHash(spooky_hash_v2::SpookyHash& hash_generator, uint64_t seed, std::string const& str) :
    m_string{ str },
    m_hash{ hash_generator.Hash64(str.c_str(), str.length(), seed) }
{

}

bool Globals::StringAndHash::operator<=(StringAndHash const& other) const
{
    if (m_hash == other.m_hash)
        return m_string <= other.m_string;

    return m_hash <= other.m_hash;
}

bool Globals::StringAndHash::operator==(StringAndHash const& other) const
{
    return m_hash == other.m_hash && m_string == other.m_string;
}


class Globals::impl
{
private:

    spooky_hash_v2::SpookyHash m_spooky_hash_generator;


public:

    impl()
    {
        long long seed1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        long long seed2 = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        seed2 = (seed2 << 32) | (seed2 >> 32);

        m_spooky_hash_generator.Init(seed1, seed2);
    }


    StringAndHash attachHasToString(std::string const& str)
    {
        long long seed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        return StringAndHash{ m_spooky_hash_generator, seed, str };
    }
};



Globals::Globals():
    m_impl{ new impl }
{

}

void* Globals::get(char const* object_type_name, void* p_object)
{
    StringAndHash hashed_string = m_impl->attachHasToString(object_type_name);

}

void const * Globals::get(char const* object_type_name) const
{
    return nullptr;
}

Globals::~Globals() = default;

Globals* Globals::create()
{
    if (!m_p_self) m_p_self = new Globals{};
    return m_p_self;
}

void Globals::destroy()
{
    if (m_p_self)
    {
        delete m_p_self;
        m_p_self = nullptr;
    }
}


