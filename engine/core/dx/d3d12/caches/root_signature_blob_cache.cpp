#include "engine/core/exception.h"
#include "engine/core/globals.h"
#include "engine/core/misc/strict_weak_ordering.h"
#include "engine/core/dx/d3d12/tasks/root_signature_builder.h"

#include "engine/core/gpu_data_blob_cache_key.h"

#include "root_signature_blob_cache.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::caches;



std::string RootSignatureBlobCache::Key::toString() const
{
    return std::string{ rs_cache_name + std::to_string(uid) + "__ROOTSIGNATURE" };
}

void RootSignatureBlobCache::Key::serialize(void* p_serialization_blob) const
{
    uint8_t* ptr = static_cast<uint8_t*>(p_serialization_blob);
    strcpy_s(reinterpret_cast<char*>(ptr), max_rs_cache_name_length, rs_cache_name); ptr += max_rs_cache_name_length;
    memcpy(ptr, &uid, sizeof(uint64_t));
}

void RootSignatureBlobCache::Key::deserialize(void const* p_serialization_blob)
{
    uint8_t const* ptr = static_cast<uint8_t const*>(p_serialization_blob);
    strcpy_s(rs_cache_name, max_rs_cache_name_length, reinterpret_cast<char const*>(ptr)); ptr += max_rs_cache_name_length;
    memcpy(&uid, ptr, sizeof(uint64_t));
}

RootSignatureBlobCache::Key::Key(std::string const& root_signature_cache_name, uint64_t uid) :
    uid{ uid }
{
    strcpy_s(rs_cache_name, max_rs_cache_name_length, root_signature_cache_name.c_str());
}

bool RootSignatureBlobCache::Key::operator<(Key const& other) const
{
    SWO_STEP(std::strcmp(rs_cache_name, other.rs_cache_name), < , 0);
    SWO_END(uid, < , other.uid);
}

bool RootSignatureBlobCache::Key::operator==(Key const& other) const
{
    return std::strcmp(rs_cache_name, other.rs_cache_name) == 0
        && uid == other.uid;
}

tasks::RootSignatureBuilder* RootSignatureBlobCache::findOrCreateTask(
    Globals& globals,
    VersionedRootSignature versioned_root_signature, RootSignatureFlags const& flags,
    std::string const& root_signature_cache_name, uint64_t uid)
{
    Key key{ root_signature_cache_name, uid };
    GpuDataBlobCacheKey combined_key{ key };

    tasks::RootSignatureBuilder* new_rs_builder{ nullptr };

    if (m_task_keys.find(combined_key) == m_task_keys.end())
    {
        auto rs_cache_keys_insertion_position =
            m_task_keys.insert(std::make_pair(combined_key, cache_storage::iterator{})).first;

        m_rs_builders.emplace_back(rs_cache_keys_insertion_position->first,
            globals, versioned_root_signature.root_signature(), flags, versioned_root_signature.timestamp());

        cache_storage::iterator p = --m_rs_builders.end();
        rs_cache_keys_insertion_position->second = p;

        tasks::RootSignatureBuilder& builder_ref = *p;
        new_rs_builder = &builder_ref;
    }
    else
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
            "Root signature with key \"" + key.toString() + "\" already exists in root signature cache. "
            "Make sure that each root signature is assigned unique cache name and identifier");
    }

    return new_rs_builder;
}

RootSignatureBlobCache::cache_storage& RootSignatureBlobCache::storage()
{
    return m_rs_builders;
}

RootSignatureBlobCache::cache_storage const& RootSignatureBlobCache::storage() const
{
    return m_rs_builders;
}
