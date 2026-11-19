#ifndef LEXGINE_CORE_DX_D3D12_DEBUG_TEXTURES_REGISTRY_H
#define LEXGINE_CORE_DX_D3D12_DEBUG_TEXTURES_REGISTRY_H

#include <vector>

// TODO: for now we only support d3d12, so we include d3d12 resources directly. In future, we need to establish API agnostic barriers
#include "engine/core/dx/d3d12/resource.h"
#include "engine/core/ui/ui_provider.h"

namespace lexgine::core {

class DebugTexturesRegistry final : public ui::UIProvider
{
public:
    void registerDebugTexture(dx::d3d12::Resource const& debug_texture) { m_debug_textures.push_back(debug_texture); }
    std::vector<dx::d3d12::Resource> const& resources() const { return m_debug_textures; }
    void imguiDrawDebugTextures() const;

private:
    std::vector<dx::d3d12::Resource> m_debug_textures;
};

}

#endif