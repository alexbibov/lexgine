#include "swap_chain_link.h"

#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/dx/dxgi/swap_chain.h"

#include "device.h"
#include "heap.h"
#include "heap_resource_placer.h"
#include "rendering_tasks.h"

using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;

SwapChainLink::SwapChainLink(Globals& globals, dxgi::SwapChain const& swap_chain_to_link,
    SwapChainDepthBufferFormat depth_buffer_format) :
    m_globals{ globals },
    m_device{ *m_globals.get<Device>() },
    m_linked_swap_chain{ swap_chain_to_link },
    m_depth_buffer_native_format{ static_cast<DXGI_FORMAT>(depth_buffer_format) },
    m_depth_optimized_clear_value{ m_depth_buffer_native_format, math::Vector4f{0.f, 0.f, 0.f, 0.f} },
    m_linked_rendering_tasks_ptr{ nullptr },
    m_expected_frame_index{ 0U }
{
    uint16_t queued_frames_count = m_globals.get<GlobalSettings>()->getMaxFramesInFlight();

    // initialize depth buffers
    {
        math::Vector2u back_buffer_dimensions = swap_chain_to_link.getDimensions();

        auto descriptor =
            ResourceDescriptor::CreateTexture2D(back_buffer_dimensions.x, back_buffer_dimensions.y,
                queued_frames_count, m_depth_buffer_native_format, 1, ResourceFlags::enum_type::depth_stencil);

        uint64_t target_heap_size = descriptor.getAllocationSize(m_device, 0x1);
        m_depth_buffer_heap.reset(new Heap{ m_device.createHeap(AbstractHeapType::default, target_heap_size,
            HeapCreationFlags::enum_type::allow_only_rt_ds, false, 0x1, 0x1) });

        HeapResourcePlacer placer{ *m_depth_buffer_heap };
        auto depth_buffer_resource = placer.addResource(ResourceState::enum_type::depth_read, m_depth_optimized_clear_value, descriptor);
        
        m_depth_buffers.reserve(queued_frames_count);
        for (uint16_t i = 0U; i < queued_frames_count; ++i)
        {
            TargetDescriptor desc;
            desc.target_resource = depth_buffer_resource;
            desc.target_mipmap_level = 0;
            desc.target_array_layer = i;
            desc.target_initial_state = ResourceState::enum_type::depth_read;
            m_depth_buffers.push_back(desc);
            m_dsv_descriptors.emplace_back(m_depth_buffers.back().target_resource, DSVTextureArrayInfo{ 0, i, 1 });
        }
        m_depth_rendering_target.reset(new RenderingTargetDepth{ m_globals, m_depth_buffers, m_dsv_descriptors });
    }

    // initialize rendering targets
    {
        m_color_buffers.reserve(queued_frames_count);
        for (uint16_t i = 0U; i < queued_frames_count; ++i)
        {
            TargetDescriptor desc;
            desc.target_resource = m_linked_swap_chain.getBackBuffer(i);
            desc.target_initial_state = ResourceState::enum_type::common;
            m_color_buffers.push_back(desc);
            m_rtv_descriptors.emplace_back(m_color_buffers.back().target_resource, RTVTextureInfo{});
        }

        m_color_rendering_target.reset(new RenderingTargetColor{ m_globals, m_color_buffers, m_rtv_descriptors });
    }
}

SwapChainLink::~SwapChainLink() = default;

void SwapChainLink::linkRenderingTasks(RenderingTasks* p_rendering_loop_to_link)
{
    m_linked_rendering_tasks_ptr = p_rendering_loop_to_link;
    m_linked_rendering_tasks_ptr->setRenderingTargets(m_color_rendering_target, m_depth_rendering_target);
}

void SwapChainLink::beginRenderingLoop()
{
    m_rendering_tasks_producer_thread = std::thread{ &RenderingTasks::run, m_linked_rendering_tasks_ptr };
    m_rendering_tasks_producer_thread.detach();
}

void SwapChainLink::dispatchExitSignal()
{
    m_linked_rendering_tasks_ptr->dispatchExitSignal();
    m_rendering_tasks_producer_thread.join();
}

void SwapChainLink::present()
{
    m_linked_rendering_tasks_ptr->waitUntilFrameIsReady(++m_expected_frame_index);
    m_linked_swap_chain.present();
}
