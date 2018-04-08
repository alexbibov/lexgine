#include "root_signature_compilation_task_cache.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/misc/strict_weak_ordering.h"
#include "lexgine/core/dx/d3d12/tasks/root_signature_compilation_task.h"

#include "combined_cache_key.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::task_caches;



std::string RootSignatureCompilationTaskCache::Key::toString() const
{
    return std::string{ "{NAME=" } +rs_cache_name + " UID=" + std::to_string(uid) + "}";
}

void RootSignatureCompilationTaskCache::Key::serialize(void* p_serialization_blob) const
{
    uint8_t* ptr = static_cast<uint8_t*>(p_serialization_blob);
    strcpy_s(reinterpret_cast<char*>(ptr), max_rs_cache_name_length, rs_cache_name); ptr += max_rs_cache_name_length;
    memcpy(ptr, &uid, sizeof(uid));
}

void RootSignatureCompilationTaskCache::Key::deserialize(void const* p_serialization_blob)
{
    uint8_t const* ptr = static_cast<uint8_t const*>(p_serialization_blob);
    strcpy_s(rs_cache_name, max_rs_cache_name_length, reinterpret_cast<char const*>(ptr)); ptr += max_rs_cache_name_length;
    memcpy(&uid, ptr, sizeof(uid));
}

RootSignatureCompilationTaskCache::Key::Key(std::string const& root_signature_cache_name, uint64_t uid) :
    uid{ uid }
{
    strcpy_s(rs_cache_name, max_rs_cache_name_length, root_signature_cache_name.c_str());
}

bool RootSignatureCompilationTaskCache::Key::operator<(Key const& other) const
{
    SWO_STEP(std::strcmp(rs_cache_name, other.rs_cache_name), < , 0);
    SWO_END(uid, < , other.uid);
}

bool RootSignatureCompilationTaskCache::Key::operator==(Key const& other) const
{
    return std::strcmp(rs_cache_name, other.rs_cache_name) == 0
        && uid == other.uid;
}

tasks::RootSignatureCompilationTask* RootSignatureCompilationTaskCache::addTask(
    Globals const& globals,
    RootSignature&& root_signature, RootSignatureFlags const& flags,
    std::string const& root_signature_cache_name, uint64_t uid)
{
    Key key{ root_signature_cache_name, uid };
    CombinedCacheKey combined_key{ key };

    tasks::RootSignatureCompilationTask* new_rs_compilation_task{ nullptr };

    if (m_task_keys.find(combined_key) == m_task_keys.end())
    {
        auto rs_cache_keys_insertion_position =
            m_task_keys.insert(std::make_pair(combined_key, cache_storage::iterator{})).first;

        m_rs_compilation_tasks.emplace_back(rs_cache_keys_insertion_position->first,
            *globals.get<GlobalSettings>(),
            std::move(root_signature), flags);

        cache_storage::iterator p = --m_rs_compilation_tasks.end();
        rs_cache_keys_insertion_position->second = p;

        tasks::RootSignatureCompilationTask& task_ref = *p;
        new_rs_compilation_task = &task_ref;
    }
    else
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
            "Root signature with key \"" + key.toString() + "\" already exists in root signature cache. "
            "Make sure that each root signature is assigned unique cache name and identifier");
    }

    return new_rs_compilation_task;
}