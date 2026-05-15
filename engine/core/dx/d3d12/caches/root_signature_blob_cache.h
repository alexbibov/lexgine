#ifndef LEXGINE_CORE_DX_D3D12_CACHES_ROOT_SIGNATURE_BLOB_CACHE_H
#define LEXGINE_CORE_DX_D3D12_CACHES_ROOT_SIGNATURE_BLOB_CACHE_H

#include "engine/core/entity.h"
#include "engine/core/class_names.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/root_signature.h"
#include "engine/core/misc/datetime.h"

#include <list>
#include <map>

namespace lexgine::core::dx::d3d12::caches {

class RootSignatureBlobCache : public NamedEntity<class_names::D3D12_RootSignatureBlobCache>
{
    friend class tasks::RootSignatureBuilder;
    friend class core::GpuDataBlobCacheKey;

public:
    struct Key
    {

    };

    class VersionedRootSignature final
    {
    public:
        VersionedRootSignature(RootSignature&& root_signature)
            : m_root_signature{ std::move(root_signature) }
            , m_timestamp{ misc::DateTime::buildTime() }
        {

        }

        RootSignature&& root_signature() { return std::move(m_root_signature); }

        misc::DateTime const& timestamp() const { return m_timestamp; }

    private:
        RootSignature m_root_signature;
        misc::DateTime m_timestamp;
    };

public:
    Key const& materializeRootSignature(
        Globals& globals,
        VersionedRootSignature versioned_root_signature, RootSignatureFlags const& flags,
        std::string const& root_signature_cache_name, uint64_t uid);

private:
    GpuDataBlobCache& m_gpu_blog_cache;
    std::unordered_map<Key, Microsoft::WRL::ComPtr<ID3D12RootSignature>> m_root_signature_cache;
};

}

#endif
