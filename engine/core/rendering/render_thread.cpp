#include <chrono>

#include "engine/core/concurrency/thread_affinity.h"
#include "engine/core/dx/d3d12/swap_chain_link.h"
#include "engine/osinteraction/windows/window.h"

#include "render_thread.h"


namespace lexgine::core {

RenderThread::RenderThread(osinteraction::windows::Window& window, dx::d3d12::SwapChainLink& swap_chain_link)
    : m_window{ window }
    , m_swap_chain_link{ swap_chain_link }
{

}

RenderThread::~RenderThread()
{
    stop();
}

void RenderThread::start()
{
    assert(!m_thread.joinable());

    m_stop_requested.store(false, std::memory_order_release);
    m_thread = std::thread{ &RenderThread::loop, this };

#ifdef _WIN32
    HANDLE threadNativeHandle = m_thread.native_handle();
    SetThreadDescription(threadNativeHandle, L"Master render thread");
#endif
}

void RenderThread::stop()
{
    if (!m_thread.joinable()) return;

    m_stop_requested.store(true, std::memory_order_release);
    m_thread.join();
}

void RenderThread::loop()
{
    concurrency::registerMasterWorkerThread();

    while (!m_stop_requested.load(std::memory_order_acquire))
    {
        for (osinteraction::windows::WindowEvent const& event : m_window.takePendingEvents())
        {
            switch (event.type)
            {
            case osinteraction::windows::WindowEventType::resized:
            case osinteraction::windows::WindowEventType::maximized:
                m_swap_chain_link.requestResize(event.width, event.height);
                break;

            case osinteraction::windows::WindowEventType::minimized:
                m_swap_chain_link.suspendRendering();
                break;

            default:
                break;
            }
        }

        // a suspended output paces nothing, so the loop would otherwise spin through the frames it skips
        if (!m_swap_chain_link.render()) std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
    }

    m_swap_chain_link.flushRenderingTasks();
}

}
