#include "render_output.h"

#include "engine/core/globals.h"
#include "engine/core/dx/dxgi/swap_chain.h"

#include "device.h"
#include "frame_progress_tracker.h"

using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;


namespace {

DXGI_FORMAT getValidDepthStencilFormatFromTypelessFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM:
        return DXGI_FORMAT_D16_UNORM;

    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
        return DXGI_FORMAT_D32_FLOAT;

    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;

    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    }
    return DXGI_FORMAT_UNKNOWN;
}

}  // namespace


RenderOutput::RenderOutput(Globals& globals, dxgi::SwapChain& swap_chain,
    SwapChainDepthBufferFormat depth_buffer_format)
    : m_globals{ globals }
    , m_swap_chain{ swap_chain }
    , m_depth_buffer_native_format{ static_cast<DXGI_FORMAT>(depth_buffer_format) }
    , m_depth_optimized_clear_value{ m_depth_buffer_native_format, math::Vector4f{1.f, 0.f, 0.f, 0.f} }
{

}

RenderOutput::RenderOutput(RenderOutput&& other)
    : m_globals{ other.m_globals }
    , m_swap_chain{ other.m_swap_chain }
    , m_buffers_acquired{ other.m_buffers_acquired }
    , m_pending_resize{ other.m_pending_resize }
    , m_pending_width{ other.m_pending_width }
    , m_pending_height{ other.m_pending_height }
    , m_color_buffers{ std::move(other.m_color_buffers) }
    , m_depth_buffer{ std::move(other.m_depth_buffer) }
    , m_depth_buffer_native_format{ other.m_depth_buffer_native_format }
    , m_depth_optimized_clear_value{ std::move(other.m_depth_optimized_clear_value) }
    , m_targets{ std::move(other.m_targets) }
{

}

void RenderOutput::requestResize(uint32_t new_width, uint32_t new_height)
{
    m_pending_width = new_width;
    m_pending_height = new_height;
    m_pending_resize = true;
}

bool RenderOutput::applyPendingResize(FrameProgressTracker const& frame_progress_tracker)
{
    if (!m_pending_resize) return false;

    // a minimized window has no client area to resize to, so the request is kept until the window is back
    if (!m_pending_width || !m_pending_height) return false;

    m_pending_resize = false;

    releaseBuffers(frame_progress_tracker);
    m_swap_chain.resizeBuffers(math::Vector2u{ m_pending_width, m_pending_height });
    acquireBuffers(m_pending_width, m_pending_height);

    return true;
}

void RenderOutput::releaseBuffers(FrameProgressTracker const& frame_progress_tracker)
{
    // there is nothing to await before the very first frame has been scheduled
    if (frame_progress_tracker.started()) frame_progress_tracker.synchronize();

    m_color_buffers.clear();
    m_depth_buffer.reset(nullptr);
    m_buffers_acquired = false;
}

void RenderOutput::acquireBuffers(uint32_t width, uint32_t height)
{
    uint16_t back_buffers_count = m_swap_chain.backBufferCount();

    // depth is never presented and the frames serialise on the direct queue, so one buffer serves every
    // back buffer
    auto descriptor =
        ResourceDescriptor::createTexture2D(width, height, 1,
            m_depth_buffer_native_format, 1, ResourceFlags::base_values::depth_stencil);

    m_depth_buffer = std::make_unique<CommittedResource>(m_globals.device(), ResourceState::base_values::depth_read,
        m_depth_optimized_clear_value, descriptor, AbstractHeapType::_default,
        HeapCreationFlags::base_values::allow_all, 0x1, 0x1);

    m_color_buffers.reserve(back_buffers_count);
    for (uint16_t i = 0U; i < back_buffers_count; ++i)
    {
        m_color_buffers.emplace_back(m_swap_chain.getBackBuffer(i));
    }

    // the rendering targets outlive the buffers they point at, so a resize repoints the descriptor tables
    // they already own instead of allocating a fresh set out of the shared RTV/DSV heaps
    bool build_targets = m_targets.size() != back_buffers_count;
    if (build_targets)
    {
        m_targets.clear();
        m_targets.reserve(back_buffers_count);
    }

    for (uint16_t i = 0U; i < back_buffers_count; ++i)
    {
        RTVTextureInfo rtv_texture_info{};
        ColorTarget color_target{ m_color_buffers[i], ResourceState::base_values::common, rtv_texture_info };

        DSVTextureInfo dsv_texture_info{};
        dsv_texture_info.mip_level_slice = 0;
        DepthTarget depth_target{ *m_depth_buffer, ResourceState::base_values::depth_read, dsv_texture_info };
        depth_target.target_view.overrideFormat(getValidDepthStencilFormatFromTypelessFormat(m_depth_buffer_native_format));

        if (build_targets)
        {
            m_targets.emplace_back(m_globals, std::vector<ColorTarget>{ color_target },
                misc::makeOptional<DepthTarget>(depth_target));
        }
        else
        {
            m_targets[i].updateRenderingTargets(std::vector<ColorTarget>{ color_target },
                misc::makeOptional<DepthTarget>(depth_target));
        }
    }

    m_buffers_acquired = true;
}

RenderingTarget& RenderOutput::currentTarget()
{
    return m_targets[m_swap_chain.getCurrentBackBufferIndex()];
}
