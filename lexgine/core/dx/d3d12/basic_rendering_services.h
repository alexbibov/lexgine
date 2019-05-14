#ifndef LEXGINE_CORE_DX_D3D12_BASIC_RENDERING_SERVICES_H
#define LEXGINE_CORE_DX_D3D12_BASIC_RENDERING_SERVICES_H

#include "lexgine/core/math/vector_types.h"
#include "lexgine/core/misc/static_vector.h"
#include "lexgine/core/viewport.h"

#include "lexgine_core_dx_d3d12_fwd.h"
#include "constant_buffer_stream.h"
#include "command_list.h"

namespace lexgine::core::dx::d3d12 {

template<typename T> class BasicRenderingServicesAttorney;

class BasicRenderingServices final
{
    friend class BasicRenderingServicesAttorney<RenderingTasks>;

public:
    BasicRenderingServices(Globals& globals);

    void beginRendering(CommandList& command_list) const;
    void endRendering(CommandList& command_list) const;

    void setDefaultViewport(CommandList& command_list) const;
    void setDefaultRenderingTarget(CommandList& command_list) const;
    void clearDefaultRenderingTarget(CommandList& command_list,
        math::Vector4f const& color_clear_value = math::Vector4f{ 0.f, 0.f, 0.f, 0.f },
        float depth_clear_value = 1.f) const;

    ConstantBufferStream& constantDataStream() { return m_constant_data_stream; }
    DedicatedUploadDataStreamAllocator& resourceUploadAllocator() { return m_resource_upload_allocator; }

private:
    void defineRenderingTarget(RenderingTarget& rendering_target) { m_current_rendering_target_ptr = &rendering_target; }

    void defineRenderingTargetFormat(DXGI_FORMAT rendering_target_color_format, DXGI_FORMAT rendering_target_depth_format)
    {
        m_rendering_target_color_format = rendering_target_color_format;
        m_rendering_target_depth_format = rendering_target_depth_format;
    }

    void defineRenderingViewport(Viewport const& viewport);

private:
    Device& m_device;
    DxResourceFactory& m_dx_resources;

    RenderingTarget* m_current_rendering_target_ptr;
    DXGI_FORMAT m_rendering_target_color_format;
    DXGI_FORMAT m_rendering_target_depth_format;

    misc::StaticVector<Viewport, CommandList::c_maximal_viewport_count> m_default_viewports;
    misc::StaticVector<math::Rectangle, CommandList::c_maximal_viewport_count> m_default_scissor_rectangles;

    misc::StaticVector<DescriptorHeap const*, 4U> m_page0_descriptor_heaps;
    ConstantBufferStream m_constant_data_stream;
    DedicatedUploadDataStreamAllocator m_resource_upload_allocator;
};

template<> class BasicRenderingServicesAttorney<RenderingTasks>
{
    friend class RenderingTasks;

    static void defineRenderingTarget(BasicRenderingServices& basic_rendering_services, RenderingTarget& rendering_target)
    {
        basic_rendering_services.defineRenderingTarget(rendering_target);
    }

    static void defineRenderingTargetFormat(BasicRenderingServices& basic_rendering_services,
        DXGI_FORMAT rendering_target_color_format, DXGI_FORMAT rendering_target_depth_format)
    {
        basic_rendering_services.defineRenderingTargetFormat(rendering_target_color_format, rendering_target_depth_format);
    }

    static void defineRenderingViewport(BasicRenderingServices& basic_rendering_services, Viewport const& viewport)
    {
        basic_rendering_services.defineRenderingViewport(viewport);
    }
};

}

#endif
