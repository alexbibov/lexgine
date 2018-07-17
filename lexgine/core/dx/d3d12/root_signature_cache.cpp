#include "root_signature_cache.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/data_blob.h"
#include "lexgine/core/dx/d3d12/device.h"

using namespace lexgine::core::dx::d3d12;

RootSignatureCache::RootSignatureCache(Device const& device):
    m_device{ device }
{

}

Microsoft::WRL::ComPtr<ID3D12RootSignature> lexgine::core::dx::d3d12::RootSignatureCache::findOrCreate(std::string const& root_signature_friendly_name, uint32_t node_mask,
    lexgine::core::D3DDataBlob const& serialized_root_signature)
{
    std::lock_guard<std::mutex> sentry{ m_lock };

    key_type key{ root_signature_friendly_name, node_mask };
    auto q = m_cached_root_signatures.find(key);

    if (q != m_cached_root_signatures.end()) return q->second;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rs{};
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_device.native()->CreateRootSignature(node_mask, serialized_root_signature.data(), serialized_root_signature.size(), IID_PPV_ARGS(&rs)),
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
