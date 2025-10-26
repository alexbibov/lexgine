#include "swap_chain_link.h"

#include "engine/core/globals.h"
#include "engine/core/global_settings.h"
#include "engine/core/dx/dxgi/swap_chain.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"

#include "device.h"
#include "heap.h"
#include "heap_resource_placer.h"
#include "rendering_tasks.h"
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

}


SwapChainLink::SwapChainLink(Globals& globals, dxgi::SwapChain& swap_chain_to_link,
    SwapChainDepthBufferFormat depth_buffer_format)
    : m_globals{ globals }
    , m_global_settings{ *globals.get<GlobalSettings>() }
    , m_device{ *m_globals.get<Device>() }
    , m_linked_swap_chain{ swap_chain_to_link }
    , m_linked_rendering_tasks_ptr{ nullptr }
    , m_depth_buffer_native_format{ static_cast<DXGI_FORMAT>(depth_buffer_format) }
    , m_depth_optimized_clear_value{ m_depth_buffer_native_format, math::Vector4f{1.f, 0.f, 0.f, 0.f} }
{

}

SwapChainLink::SwapChainLink(SwapChainLink&& other)
    : m_globals{ other.m_globals }
    , m_global_settings{ other.m_global_settings }
    , m_device{ other.m_device }
    , m_linked_swap_chain{ other.m_linked_swap_chain }
    , m_linked_rendering_tasks_ptr{ other.m_linked_rendering_tasks_ptr }
    , m_color_buffers{ std::move(other.m_color_buffers) }
    , m_depth_buffer{ std::move(other.m_depth_buffer) }
    , m_depth_buffer_native_format{ other.m_depth_buffer_native_format }
    , m_depth_optimized_clear_value{ std::move(other.m_depth_optimized_clear_value) }
    , m_targets{ std::move(other.m_targets) }
{

}

std::shared_ptr<SwapChainLink> SwapChainLink::create(Globals& globals, dxgi::SwapChain& swap_chain_to_link, SwapChainDepthBufferFormat depth_buffer_format)
{
    std::shared_ptr<SwapChainLink> rv{ new SwapChainLink{globals, swap_chain_to_link, depth_buffer_format} };
    swap_chain_to_link.window().addListener(rv);
    return rv;
}

SwapChainLink::~SwapChainLink()
{
    if (m_linked_rendering_tasks_ptr) m_linked_rendering_tasks_ptr->flush();
}

void SwapChainLink::linkRenderingTasks(RenderingTasks* p_rendering_loop_to_link)
{
    m_linked_rendering_tasks_ptr = p_rendering_loop_to_link;
    updateRenderingConfiguration();
}

void SwapChainLink::render()
{
    if (m_targets.empty())
    {
        core::math::Vector2u swap_chain_dimensions = m_linked_swap_chain.getDimensions();
        acquireBuffers(swap_chain_dimensions.x, swap_chain_dimensions.y);
        updateRenderingConfiguration();
        m_suspend_rendering = false;
    }

    if (m_linked_swap_chain.isIdle()) {    // check if we should exit idle presentation state
        m_linked_swap_chain.present();
    }
    else if (!m_suspend_rendering && m_linked_rendering_tasks_ptr)
    {
        RenderingTarget& target = m_targets[m_linked_swap_chain.getCurrentBackBufferIndex()];
        FrameProgressTracker const& frame_progress_tracker = m_linked_rendering_tasks_ptr->frameProgressTracker();

        uint64_t scheduled_frames_count = frame_progress_tracker.scheduledFramesCount();
        uint64_t completed_frames_count = frame_progress_tracker.completedFramesCount();
        while (scheduled_frames_count - completed_frames_count >= m_global_settings.getMaxFramesInFlight())
        {
            frame_progress_tracker.waitForFrameCompletion(completed_frames_count);
            ++completed_frames_count;
        }

        m_linked_rendering_tasks_ptr->render(target, [this](void) { m_linked_swap_chain.present(); });
    }
}

bool SwapChainLink::minimized()
{
    m_suspend_rendering = true;
    return true;
}

bool SwapChainLink::maximized(uint16_t new_width, uint16_t new_height)
{
    return size_changed(new_width, new_height);
}

bool SwapChainLink::size_changed(uint16_t new_width, uint16_t new_height)
{
    if (m_linked_rendering_tasks_ptr)
    {
        releaseBuffers();
        m_linked_swap_chain.resizeBuffers(math::Vector2u{ new_width, new_height });
        acquireBuffers(new_width, new_height);
        updateRenderingConfiguration();
        m_suspend_rendering = false;
    }

    return true;
}

void SwapChainLink::updateRenderingConfiguration() const
{
    math::Vector2u swap_chain_dimensions = m_linked_swap_chain.getDimensions();
    Viewport viewport{ math::Vector2f{0.f, 0.f}, math::Vector2f{ static_cast<float>(swap_chain_dimensions.x),
        static_cast<float>(swap_chain_dimensions.y)},
    math::Vector2f{0, 1.f} };

    m_linked_rendering_tasks_ptr->defineRenderingConfiguration(RenderingConfiguration{
        viewport,
        m_linked_swap_chain.descriptor().format, m_depth_buffer_native_format,
        &m_linked_swap_chain.window() });
}

void SwapChainLink::releaseBuffers()
{
    FrameProgressTracker const& frame_progress_tracker = m_linked_rendering_tasks_ptr->frameProgressTracker();
    frame_progress_tracker.synchronize();

    m_color_buffers.clear();
    m_depth_buffer.reset(nullptr);
    m_targets.clear();
}

void SwapChainLink::acquireBuffers(uint32_t width, uint32_t height)
{
    auto dx_resource_factory = m_globals.get<DxResourceFactory>();
    dx_resource_factory->retrieveDescriptorHeap(m_device, DescriptorHeapType::rtv, 0).reset();
    dx_resource_factory->retrieveDescriptorHeap(m_device, DescriptorHeapType::dsv, 0).reset();

    uint16_t back_buffers_count = m_linked_swap_chain.backBufferCount();
    m_color_buffers.reserve(back_buffers_count);
    m_targets.reserve(back_buffers_count);

    auto descriptor =
        ResourceDescriptor::CreateTexture2D(width, height, back_buffers_count,
            m_depth_buffer_native_format, 1, ResourceFlags::base_values::depth_stencil);

    m_depth_buffer = std::make_unique<CommittedResource>(m_device, ResourceState::base_values::depth_read,
        m_depth_optimized_clear_value, descriptor, AbstractHeapType::_default,
        HeapCreationFlags::base_values::allow_all, 0x1, 0x1);


    for (uint16_t i = 0U; i < m_linked_swap_chain.backBufferCount(); ++i)
    {
        m_color_buffers.emplace_back(m_linked_swap_chain.getBackBuffer(i));

        RTVTextureInfo rtv_texture_info{};
        ColorTarget color_target{ m_color_buffers.back(), ResourceState::base_values::common, rtv_texture_info };

        DSVTextureArrayInfo dsv_texture_array_info{};
        dsv_texture_array_info.mip_level_slice = 0;
        dsv_texture_array_info.first_array_element = i;
        dsv_texture_array_info.num_array_elements = 1;
        DepthTarget depth_target{ *m_depth_buffer, ResourceState::base_values::depth_read, dsv_texture_array_info };
        depth_target.target_view.overrideFormat(getValidDepthStencilFormatFromTypelessFormat(m_depth_buffer_native_format));

        m_targets.emplace_back(m_globals, std::vector<ColorTarget>{ color_target }, misc::makeOptional<DepthTarget>(depth_target));
    }
}
