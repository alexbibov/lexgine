#include <chrono>
#include <type_traits>

#include "engine/core/concurrency/thread_affinity.h"
#include "engine/core/dx/d3d12/swap_chain_link.h"
#include "engine/osinteraction/windows/window.h"

#include "render_thread.h"


namespace lexgine::core::rendering {

namespace {

/*! How long the loop idles when a frame is not worth rendering. The intervals are judgement calls rather
 than derived values: an obscured or minimized output costs nothing but the delay before it is noticed to be
 back.
*/
constexpr std::chrono::milliseconds c_occluded_poll_interval{ 50 };
constexpr std::chrono::milliseconds c_suspended_poll_interval{ 16 };

}  // namespace

std::shared_ptr<RenderThread> RenderThread::create(osinteraction::windows::Window& window,
    dx::d3d12::SwapChainLink& swap_chain_link, std::function<void()> device_lost_handler)
{
    std::shared_ptr<RenderThread> rv{ new RenderThread{ swap_chain_link, std::move(device_lost_handler) } };
    if (IsIconic(window.native())) rv->m_surface_events.record(SuspendEvent{});
    window.addListener(rv);
    return rv;
}

RenderThread::RenderThread(dx::d3d12::SwapChainLink& swap_chain_link, std::function<void()> device_lost_handler)
    : m_swap_chain_link{ swap_chain_link }
    , m_device_lost_handler{ std::move(device_lost_handler) }
{

}

RenderThread::~RenderThread()
{
    stop();
}

void RenderThread::start()
{
    assert(!m_thread.joinable());

    m_stop_event.reset();
    m_thread = std::thread{ &RenderThread::loop, this };

#ifdef _WIN32
    HANDLE threadNativeHandle = m_thread.native_handle();
    SetThreadDescription(threadNativeHandle, L"Master render thread");
#endif
}

void RenderThread::stop()
{
    if (!m_thread.joinable()) return;

    m_stop_event.set();
    m_thread.join();
}

bool RenderThread::minimized()
{
    m_surface_events.record(SuspendEvent{});
    return true;
}

bool RenderThread::maximized(uint16_t new_width, uint16_t new_height)
{
    m_surface_events.record(ResizeEvent{ new_width, new_height });
    return true;
}

bool RenderThread::size_changed(uint16_t new_width, uint16_t new_height)
{
    m_surface_events.record(ResizeEvent{ new_width, new_height });
    return true;
}

void RenderThread::loop()
{
    concurrency::registerMasterWorkerThread();

    bool device_lost = false;
    while (!device_lost && !m_stop_event.wait(0))
    {
        for (SurfaceEvent const& surface_event : m_surface_events.take())
        {
            std::visit([this](auto const& e)
                {
                    using EventType = std::decay_t<decltype(e)>;

                    if constexpr (std::is_same_v<EventType, ResizeEvent>)
                    {
                        m_swap_chain_link.requestResize(e.width, e.height);
                    }
                    else if constexpr (std::is_same_v<EventType, SuspendEvent>)
                    {
                        m_swap_chain_link.suspendRendering();
                    }
                }, surface_event);
        }

        // only a rendered frame paces the loop; the rest describe how long there is nothing worth doing
        switch (m_swap_chain_link.render(m_stop_event.native()))
        {
        case dx::d3d12::FrameOutcome::rendered:
        case dx::d3d12::FrameOutcome::interrupted:
            break;

        case dx::d3d12::FrameOutcome::occluded:
            std::this_thread::sleep_for(c_occluded_poll_interval);
            break;

        case dx::d3d12::FrameOutcome::suspended:
            std::this_thread::sleep_for(c_suspended_poll_interval);
            break;

        case dx::d3d12::FrameOutcome::device_lost:
            device_lost = true;
            m_device_lost_handler();
            break;
        }
    }

    m_swap_chain_link.flushRenderingTasks();
}

}
