#include "swap_chain_link.h"

#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/logging_streams.h"
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
    m_global_settings{ *globals.get<GlobalSettings>() },
    m_device{ *m_globals.get<Device>() },
    m_linked_swap_chain{ swap_chain_to_link },
    m_linked_rendering_tasks_ptr{ nullptr },
    m_depth_buffer_native_format{ static_cast<DXGI_FORMAT>(depth_buffer_format) },
    m_depth_optimized_clear_value{ m_depth_buffer_native_format, math::Vector4f{0.f, 0.f, 0.f, 0.f} },
    m_expected_frame_index{ 0U }
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

    m_targets.reserve(queued_frames_count);
    for (uint16_t i = 0U; i < queued_frames_count; ++i)
    {
        ColorTarget color_target{ m_linked_swap_chain.getBackBuffer(i), ResourceState::enum_type::common, RTVTextureInfo{} };
        DepthTarget depth_target{ m_depth_buffer, ResourceState::enum_type::depth_read, DSVTextureInfo{} };

        m_targets.push_back(std::make_shared<RenderingTarget>(m_globals, std::vector<ColorTarget>{ color_target }, misc::makeOptional<DepthTarget>(depth_target)));
        
    }
}

SwapChainLink::~SwapChainLink() = default;

void SwapChainLink::linkRenderingTasks(RenderingTasks* p_rendering_loop_to_link)
{
    m_linked_rendering_tasks_ptr = p_rendering_loop_to_link;
    m_linked_rendering_tasks_ptr->setRenderingTargets(m_targets);
}

void SwapChainLink::beginRenderingLoop()
{
    misc::Log::retrieve()->out("Main loop started", misc::LogMessageType::information);
    m_rendering_tasks_producer_thread = std::thread{ [this]()
    {
        auto& rendering_logging_file_stream = m_globals.get<LoggingStreams>()->rendering_logging_stream;
        misc::Log::create(rendering_logging_file_stream, "rendering_loop_log", m_global_settings.getTimeZone(), m_global_settings.isDTS());

        m_linked_rendering_tasks_ptr->run();

        misc::Log::shutdown();
    } };
    m_rendering_tasks_producer_thread.detach();
}

void SwapChainLink::dispatchExitSignal()
{
    m_linked_rendering_tasks_ptr->dispatchExitSignal();
    m_rendering_tasks_producer_thread.join();

    misc::Log::retrieve()->out("Main loop finished", misc::LogMessageType::information);
}

void SwapChainLink::present()
{
    m_linked_rendering_tasks_ptr->waitUntilFrameIsReady(++m_expected_frame_index);
    m_linked_swap_chain.present();
}
