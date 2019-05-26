#include "swap_chain_link.h"

#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/logging_streams.h"
#include "lexgine/core/dx/dxgi/swap_chain.h"

#include "device.h"
#include "heap.h"
#include "heap_resource_placer.h"
#include "rendering_tasks.h"
#include "frame_progress_tracker.h"

using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;


SwapChainLink::SwapChainLink(Globals& globals, dxgi::SwapChain const& swap_chain_to_link,
    SwapChainDepthBufferFormat depth_buffer_format)
    : m_globals{ globals }
    , m_global_settings{ *globals.get<GlobalSettings>() }
    , m_device{ *m_globals.get<Device>() }
    , m_linked_swap_chain{ swap_chain_to_link }
    , m_linked_rendering_tasks_ptr{ nullptr }
    , m_depth_buffer_native_format{ static_cast<DXGI_FORMAT>(depth_buffer_format) }
    , m_depth_optimized_clear_value{ m_depth_buffer_native_format, math::Vector4f{1.f, 0.f, 0.f, 0.f} }
    , m_presenter{ [this](RenderingTarget const&) { m_linked_swap_chain.present(); } }
{
    uint16_t queued_frames_count = m_globals.get<GlobalSettings>()->getMaxFramesInFlight();


    // initialize targets

    math::Vector2u back_buffer_dimensions = swap_chain_to_link.getDimensions();

    auto descriptor =
        ResourceDescriptor::CreateTexture2D(back_buffer_dimensions.x, back_buffer_dimensions.y,
            queued_frames_count, m_depth_buffer_native_format, 1, ResourceFlags::enum_type::depth_stencil);

    uint64_t target_heap_size = descriptor.getAllocationSize(m_device, 0x1);
    m_depth_buffer_heap.reset(new Heap{ m_device.createHeap(AbstractHeapType::default, target_heap_size,
        HeapCreationFlags::enum_type::allow_only_rt_ds, false, 0x1, 0x1) });

    HeapResourcePlacer placer{ *m_depth_buffer_heap };
    m_depth_buffer = placer.addResource(ResourceState::enum_type::depth_read, m_depth_optimized_clear_value, descriptor);

    m_color_buffers.reserve(queued_frames_count);
    m_targets.reserve(queued_frames_count);
    for (uint16_t i = 0U; i < queued_frames_count; ++i)
    {
        m_color_buffers.emplace_back(m_linked_swap_chain.getBackBuffer(i));

        RTVTextureInfo rtv_texture_info{};
        ColorTarget color_target{ m_color_buffers.back(), ResourceState::enum_type::common, rtv_texture_info };
        
        DSVTextureArrayInfo dsv_texture_array_info{};
        dsv_texture_array_info.mip_level_slice = 0;
        dsv_texture_array_info.first_array_element = i;
        dsv_texture_array_info.num_array_elements = 1;
        DepthTarget depth_target{ m_depth_buffer, ResourceState::enum_type::depth_read, dsv_texture_array_info };

        m_targets.emplace_back(m_globals, std::vector<ColorTarget>{ color_target }, misc::makeOptional<DepthTarget>(depth_target));
    }
}

SwapChainLink::SwapChainLink(SwapChainLink&& other)
    : m_globals{ other.m_globals }
    , m_global_settings{ other.m_global_settings }
    , m_device{ other.m_device }
    , m_linked_swap_chain{ other.m_linked_swap_chain }
    , m_linked_rendering_tasks_ptr{ other.m_linked_rendering_tasks_ptr }
    , m_color_buffers{ std::move(other.m_color_buffers) }
    , m_depth_buffer_heap{ std::move(other.m_depth_buffer_heap) }
    , m_depth_buffer_native_format{ other.m_depth_buffer_native_format }
    , m_depth_optimized_clear_value{ std::move(other.m_depth_optimized_clear_value) }
    , m_depth_buffer{ std::move(other.m_depth_buffer) }
    , m_targets{ std::move(other.m_targets) }
    , m_presenter{ [this](RenderingTarget const&) {m_linked_swap_chain.present(); } }
{

}

SwapChainLink::~SwapChainLink()
{
    if (m_linked_rendering_tasks_ptr) m_linked_rendering_tasks_ptr->flush();
}

void SwapChainLink::linkRenderingTasks(RenderingTasks* p_rendering_loop_to_link)
{
    m_linked_rendering_tasks_ptr = p_rendering_loop_to_link;

    math::Vector2u window_dimensions = m_linked_swap_chain.getDimensions();
    Viewport viewport{ math::Vector2f{0.f, 0.f}, math::Vector2f{ static_cast<float>(window_dimensions.x), static_cast<float>(window_dimensions.y)},
    math::Vector2f{0, 1.f} };

    
    m_linked_rendering_tasks_ptr->defineRenderingConfiguration(viewport, m_linked_swap_chain.descriptor().format, m_depth_buffer_native_format, 
        &m_linked_swap_chain.window());
}

void SwapChainLink::render()
{
    if(m_linked_rendering_tasks_ptr)
    {
        uint32_t current_back_buffer_index = m_linked_swap_chain.getCurrentBackBufferIndex();
        RenderingTarget& target = m_targets[current_back_buffer_index];
        FrameProgressTracker const& frame_progress_tracker = m_linked_rendering_tasks_ptr->frameProgressTracker();

        uint64_t frame_idx = frame_progress_tracker.currentFrameIndex();
        uint64_t competing_frame_idx = frame_idx - m_linked_swap_chain.backBufferCount();

        if (frame_idx >= m_linked_swap_chain.backBufferCount()
            && frame_progress_tracker.lastCompletedFrameIndex() < competing_frame_idx)
        {
            frame_progress_tracker.waitForFrameCompletion(competing_frame_idx);
        }

        m_linked_rendering_tasks_ptr->render(target, m_presenter);
    }

}
