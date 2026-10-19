#ifndef LEXGINE_CORE_RENDERING_RENDER_THREAD_H
#define LEXGINE_CORE_RENDERING_RENDER_THREAD_H

#include <atomic>
#include <memory>
#include <thread>
#include <variant>

#include "engine/core/concurrency/deferred_event_queue.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/osinteraction/windows/lexgine_osinteraction_windows_fwd.h"
#include "engine/osinteraction/windows/window_listeners.h"


namespace lexgine::core {

/*! Drives the frames on a thread of its own, so that the thread pumping the window messages never waits for
 the GPU. The size changes of the window are recorded as they arrive and applied by the loop, which then
 renders a frame.
*/
class RenderThread final : public osinteraction::windows::WindowSizeChangeListener
{
public:
    /*! creates the rendering thread driving @param swap_chain_link and registers it as a listener of
     @param window, whose size changes it follows. The thread is not started.
    */
    static std::shared_ptr<RenderThread> create(osinteraction::windows::Window& window,
        dx::d3d12::SwapChainLink& swap_chain_link);

public:
    ~RenderThread();

    RenderThread(RenderThread const&) = delete;
    RenderThread& operator=(RenderThread const&) = delete;

    void start();    //! begins driving the frames
    void stop();    //! asks the thread to leave its loop and waits until it does

    bool isRunning() const { return m_thread.joinable(); }    //! returns 'true' while the frames are being driven

public:    // WindowSizeChangeListener events
    bool minimized() override;
    bool maximized(uint16_t new_width, uint16_t new_height) override;
    bool size_changed(uint16_t new_width, uint16_t new_height) override;

private:
    struct ResizeEvent
    {
        uint32_t width;
        uint32_t height;
    };

    struct SuspendEvent
    {
    };

    using SurfaceEvent = std::variant<ResizeEvent, SuspendEvent>;

private:
    explicit RenderThread(dx::d3d12::SwapChainLink& swap_chain_link);

    void loop();    //! function looped by the rendering thread

private:
    dx::d3d12::SwapChainLink& m_swap_chain_link;

    concurrency::DeferredEventQueue<SurfaceEvent> m_surface_events{ c_surface_event_capacity };

    std::thread m_thread;
    std::atomic_bool m_stop_requested{ false };

private:
    static constexpr size_t c_surface_event_capacity = 256;
};

}

#endif
