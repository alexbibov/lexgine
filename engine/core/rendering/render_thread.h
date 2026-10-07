#ifndef LEXGINE_CORE_RENDERING_RENDER_THREAD_H
#define LEXGINE_CORE_RENDERING_RENDER_THREAD_H

#include <atomic>
#include <thread>

#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/osinteraction/windows/lexgine_osinteraction_windows_fwd.h"


namespace lexgine::core {

/*! Drives the frames on a thread of its own, so that the thread pumping the window messages never waits for
 the GPU. The loop drains the events recorded by the window procedure, applies them and renders a frame.
*/
class RenderThread final
{
public:
    RenderThread(osinteraction::windows::Window& window, dx::d3d12::SwapChainLink& swap_chain_link);
    ~RenderThread();

    RenderThread(RenderThread const&) = delete;
    RenderThread& operator=(RenderThread const&) = delete;

    void start();    //! begins driving the frames
    void stop();    //! asks the thread to leave its loop and waits until it does

    bool isRunning() const { return m_thread.joinable(); }    //! returns 'true' while the frames are being driven

private:
    void loop();    //! function looped by the rendering thread

private:
    osinteraction::windows::Window& m_window;
    dx::d3d12::SwapChainLink& m_swap_chain_link;

    std::thread m_thread;
    std::atomic_bool m_stop_requested{ false };
};

}

#endif
