#ifndef LEXGINE_CORE_DX_D3D12_ROOT_SIGNATURE_CACHE_H
#define LEXGINE_CORE_DX_D3D12_ROOT_SIGNATURE_CACHE_H

#include <wrl.h>
#include <d3d12.h>

#include <unordered_map>
#include <mutex>

#include "lexgine/core/misc/hashed_string.h"
#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/entity.h"


namespace lexgine::core::dx::d3d12 {

class RootSignatureCache : public NamedEntity<class_names::D3D12_RootSignatureCache>
{
    friend class Device;    // only device classes are allowed to create root signature low-level cache

private:
    RootSignatureCache() = default;
    RootSignatureCache(RootSignatureCache const&) = delete;
    RootSignatureCache(RootSignatureCache&&) = delete;    // cannot move because of the mutex

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> findOrCreate(
        Device const& device,
        std::string const& root_signature_friendly_name, uint32_t node_mask, 
        lexgine::core::D3DDataBlob const& serialized_root_signature);

    Microsoft::WRL::ComPtr<ID3D12RootSignature> find(
        std::string const& root_signature_friendly_name, uint32_t node_mask) const;

private:
    using key_type = std::pair<std::string, uint32_t>;

    class key_type_hasher_and_comparator
    {
    public:
        size_t operator()(key_type const& key) const    // hasher
        {
            uint64_t hash = misc::HashedString{ key.first }.hash();
            hash ^= static_cast<uint64_t>(key.second);
            return static_cast<size_t>(hash);
        }

        bool operator()(key_type const& a, key_type const& b) const    // comparator
        {
            if (a.first < b.first) return true;
            else if (a.first > b.first) return false;
            else return a.second < b.second;
        }
    };

    using rs_cache_map = std::unordered_map<key_type, Microsoft::WRL::ComPtr<ID3D12RootSignature>, key_type_hasher_and_comparator, key_type_hasher_and_comparator>;

private:
    rs_cache_map m_cached_root_signatures;
    std::mutex m_lock;
};

}

#endif
