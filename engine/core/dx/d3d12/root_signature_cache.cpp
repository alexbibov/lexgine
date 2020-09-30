#include "root_signature_cache.h"
#include "engine/core/exception.h"
#include "engine/core/data_blob.h"
#include "engine/core/dx/d3d12/device.h"

using namespace lexgine::core::dx::d3d12;

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureCache::findOrCreate(
    Device const& device,
    std::string const& root_signature_friendly_name, uint32_t node_mask,
    lexgine::core::D3DDataBlob const& serialized_root_signature)
{
    std::lock_guard<std::mutex> sentry{ m_lock };

    key_type key{ root_signature_friendly_name, node_mask };
    auto q = m_cached_root_signatures.find(key);

    if (q != m_cached_root_signatures.end()) return q->second;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rs{};
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        device.native()->CreateRootSignature(node_mask, serialized_root_signature.data(), serialized_root_signature.size(), IID_PPV_ARGS(&rs)),
        S_OK
    );
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        rs->SetName(misc::asciiStringToWstring(root_signature_friendly_name).c_str()),
        S_OK
    );

    m_cached_root_signatures.insert(std::make_pair(key, rs));

    return rs;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureCache::find(
    std::string const& root_signature_friendly_name, uint32_t node_mask) const
{
    auto p = m_cached_root_signatures.find(key_type{ root_signature_friendly_name, node_mask });
    return p != m_cached_root_signatures.end() ? p->second : nullptr;
}
