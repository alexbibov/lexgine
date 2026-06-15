#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_GBUFFER_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_GBUFFER_H

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/globals.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/resource.h"

namespace lexgine::core::dx::d3d12::rendering
{

class Gbuffer final
{
public:
    Gbuffer(Globals& globals);
    void init(uint32_t width, uint32_t height);

private:
    MultiSamplingFormat queryMultisamplingFormatForDxgiSurface(DXGI_FORMAT dxgi_format) const;

private:
    Globals& m_globals;
    GlobalSettings& m_globals_settings;
    Device& m_device;
    std::unique_ptr<Heap> m_heap;
    std::unique_ptr<PlacedResource> m_albedo;  // RGB components of albedo (DXGI_FORMAT_R11G11B10_FLOAT)
    std::unique_ptr<PlacedResource> m_nmre;  // normals (RG), metallic (LS 8 bits of B-channel) and roughness (MS 8 bits of B-channel), and A is the red components (DXGI_FORMAT_R16G16B16A16_FLOAT)
    std::unique_ptr<PlacedResource> m_emission;  // emission intensity (RG channels contain its green and blue components) (DXGI_FORMAT_R16G16_FLOAT)
    std::unique_ptr<PlacedResource> m_depth;  // depth target (DXGI_FORMAT_D32_FLOAT)
    uint32_t m_width, m_height;
};

}

#endif