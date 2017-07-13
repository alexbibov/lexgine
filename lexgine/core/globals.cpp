#include "globals.h"

#include <chrono>

using namespace lexgine::core;



Globals* Globals::m_p_self{ nullptr };


Globals::Globals()
{
    unsigned long long seed1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    unsigned long long seed2 = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    seed2 = (seed2 << 32) | (seed2 >> 32);
    m_hash_seed = ((seed1 << 16) | (seed1 >> 48)) + seed2;

    m_spooky_hash_generator.Init(seed1, seed2);
}

void* Globals::find(misc::HashedString const& hashed_name)
{
    global_object_pool_type::iterator target_entry;
    if ((target_entry = m_global_object_pool.find(hashed_name)) == m_global_object_pool.end()) return nullptr;

    return target_entry->second;
}

void const* Globals::find(misc::HashedString const& hashed_name) const
{
    return const_cast<Globals*>(this)->find(hashed_name);
}

bool Globals::put(misc::HashedString const& hashed_name, void* p_object)
{
    return m_global_object_pool.insert(std::make_pair(hashed_name, p_object)).second;
}

misc::HashedString Globals::attachHasToString(std::string const& str)
{
    return misc::HashedString{ m_spooky_hash_generator, m_hash_seed, str };
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


