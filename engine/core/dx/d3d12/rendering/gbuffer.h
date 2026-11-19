#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_GBUFFER_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_GBUFFER_H

#include <span>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/globals.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/resource.h"
#include "engine/core/dx/d3d12/rendering_target.h"

namespace lexgine::core::dx::d3d12::rendering
{

class Gbuffer final
{
public:
    Gbuffer(Globals& globals);
    void init(uint32_t width, uint32_t height);

    //! returns the rendering target viewing the GBuffer surfaces. The default state of the color targets leaves
    //! them ready for pixel shader access, so a pass rendering into the GBuffer returns them sample-able
    RenderingTarget const& renderingTarget() const { return *m_rendering_target; }

    //! returns the albedo surface of the GBuffer
    Resource const& albedo() const { return m_albedo; }

    //! returns 'true' if the albedo target is multisampled and must be resolved before it can be sampled
    bool needsAlbedoResolve() const { return m_resolved_albedo != nullptr; }

    //! returns the albedo surface to sample: the resolve target when the albedo is multisampled, the albedo surface itself otherwise
    Resource const& sampleableAlbedo() const { return m_resolved_albedo ? m_resolved_albedo : m_albedo; }

private:
    MultiSamplingFormat queryMultisamplingFormat(std::span<DXGI_FORMAT const> dxgi_formats) const;

private:
    Globals& m_globals;
    GlobalSettings& m_globals_settings;
    Device& m_device;
    std::unique_ptr<Heap> m_heap;
    PlacedResource m_albedo;  // RGB components of albedo (DXGI_FORMAT_R11G11B10_FLOAT)
    PlacedResource m_nmre;  // normals (RG), metallic (LS 8 bits of B-channel) and roughness (MS 8 bits of B-channel), and A is the red components (DXGI_FORMAT_R16G16B16A16_FLOAT)
    PlacedResource m_emission;  // emission intensity (RG channels contain its green and blue components) (DXGI_FORMAT_R16G16_FLOAT)
    PlacedResource m_depth;  // depth target (DXGI_FORMAT_D32_FLOAT)

    std::unique_ptr<Heap> m_resolved_heap;  // heap holding the non-multisampled copies of the GBuffer targets; exists only when MSAA is enabled
    PlacedResource m_resolved_albedo;  // non-multisampled copy of the albedo target; exists only when MSAA is enabled
    std::unique_ptr<RenderingTarget> m_rendering_target;
    uint32_t m_width, m_height;
};

}

#endif