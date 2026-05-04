#include <cassert>

#include "globals.h"
#include "global_settings.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/d3d12/task_caches/hlsl_compilation_task_cache.h"
#include "engine/core/dx/d3d12/task_caches/pso_compilation_task_cache.h"
#include "engine/core/dx/d3d12/task_caches/root_signature_compilation_task_cache.h"


using namespace lexgine::core;



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

void Globals::put(misc::HashedString const& hashed_name, void* p_object)
{
    auto it = m_global_object_pool.find(hashed_name);
    if (it != m_global_object_pool.end())
    {
        it->second = p_object;
    }
    else
    {
        m_global_object_pool.insert(std::make_pair(hashed_name, p_object)).second;
    }
}
