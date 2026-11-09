#ifndef LEXGINE_CORE_DX_DXGI_SWAP_CHAIN_H
#define LEXGINE_CORE_DX_DXGI_SWAP_CHAIN_H


#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <dxgi1_6.h>
#include <wrl.h>

#include "engine/core/dx/dxgi/lexgine_core_dx_dxgi_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/resource.h"
#include "engine/core/math/vector_types.h"
#include "engine/core/multisampling.h"


using namespace Microsoft::WRL;


namespace lexgine::core::dx::dxgi {

template<typename T> class SwapChainAttorney;

//! Describes swap chain scaling modes
enum class SwapChainScaling
{
    stretch = DXGI_SCALING_STRETCH,
    none = DXGI_SCALING_NONE,
    stretch_and_keep_aspect_ratio = DXGI_SCALING_ASPECT_RATIO_STRETCH
};


//! Outcome of a request to present the contents of the back buffer
enum class PresentResult
{
    ok,    //!< the frame was handed to the presentation engine
    occluded,    //!< the window is fully occluded; the swap chain has entered idle presentation
    device_lost,    //!< the device was removed or reset; nothing can be presented until it is recreated
    failed    //!< the presentation engine rejected the frame for some other reason
};


//! Outcome of a wait for room in the presentation queue
enum class FrameLatencyWait
{
    ready,    //!< the presentation queue has room for another frame
    timed_out,    //!< the presentation queue has not made room in time
    interrupted    //!< the wait was abandoned on request of the caller
};


//! Identifies the present most recently shown on the display
struct DisplayedPresent
{
    uint32_t present_count;    //!< number of presents made up to and including the displayed one
    std::chrono::steady_clock::time_point display_time;    //!< time of the vertical blank at which it was displayed
};


//! Progress of the presentation
struct PresentStatistics
{
    uint32_t present_count;    //!< number of presents made so far
    std::optional<DisplayedPresent> last_displayed;    //!< the present most recently displayed, if it is known
};


//! Describes parameters of the swap chain
struct SwapChainDescriptor
{
    DXGI_FORMAT format;    //!< display format
    bool stereo;
    SwapChainScaling scaling;
    bool enable_vsync;
    uint32_t back_buffer_count;

    /*! number of presents allowed to queue before the next frame is held back by SwapChain::waitForFrameLatency().
     0 keeps the default of the presentation engine and does not hold the frames back
    */
    uint32_t max_frame_latency;
};

class SwapChain final : public core::NamedEntity<SwapChain>
{
    friend class SwapChainAttorney<HwAdapter>;

public:
    SwapChain(SwapChain const&) = delete;
    SwapChain(SwapChain&&) = default;

public:

    //! Retrieves Direct3D12 device associated with the swap chain
    d3d12::Device& device() const;

    //! Retrieves graphics command queue associated with the swap chain
    d3d12::CommandQueue const& defaultCommandQueue() const;

    //! Retrieves current width and height of the swap chain packed into a 2D vector
    math::Vector2u getDimensions() const;

    //! Retrieves one of the back buffers of the swap chain and wraps it into a Resource object
    dx::d3d12::Resource getBackBuffer(uint32_t buffer_index) const;

    //! Returns index of the current back buffer of the swap chain
    uint32_t getCurrentBackBufferIndex() const;

    /*! Puts contents of the back buffer into the front buffer.

     Never throws on a presentation failure: a lost device, or any other rejection by the presentation engine,
     is logged and reported through the return value. Once the device is lost every subsequent call is a
     no-op returning PresentResult::device_lost until the device is recreated.
    */
    PresentResult present() const;

    /*! Returns the number of presents made so far together with the present most recently shown on the display.
     The latter is unknown until a present has been displayed or when the statistics have been interrupted
    */
    PresentStatistics presentStatistics() const;

    //! Total back buffer count
    uint32_t backBufferCount() const;

    //! Retrieves descriptor of the swap chain
    SwapChainDescriptor descriptor() const;

    //! Resizes the swap chain 
    void resizeBuffers(math::Vector2u const& new_dimensions);

    //! Checks if the swap chain is in idle state
    bool isIdle() const { return m_swapChainIsIdle; }

    //! Checks if the device backing the swap chain has been lost
    bool isDeviceLost() const { return m_device_lost; }

    /*! Blocks until the presentation queue has room for another frame, @param timeout expires or
     @param interrupt_event is signalled. Returns FrameLatencyWait::ready at once if the swap chain does not limit
     its frame latency
    */
    FrameLatencyWait waitForFrameLatency(std::chrono::milliseconds timeout, HANDLE interrupt_event) const;

private:
    SwapChain(ComPtr<IDXGIFactory6> const& dxgi_factory,
        d3d12::Device& device,
        d3d12::CommandQueue const& default_command_queue,
        HWND hwnd,
        math::Vector2u const& client_area_size,
        SwapChainDescriptor const& desc);

    PresentResult reportPresentFailure(HRESULT hres) const;

private:
    struct HandleCloser
    {
        void operator()(HANDLE handle) const { CloseHandle(handle); }
    };

private:
    ComPtr<IDXGIFactory6> m_dxgi_factory;   //!< DXGI factory used to create the swap chain
    d3d12::Device& m_device;    //!< Direct3D12 device interface
    HWND m_hwnd;    //!< handle of the window, which holds the swap chain
    d3d12::CommandQueue const& m_default_command_queue;    //!< graphics command queue associated with the swap chain
    SwapChainDescriptor m_descriptor;    //!< Descriptor of the swap chain
    ComPtr<IDXGISwapChain4> m_dxgi_swap_chain;   //!< DXGI interface representing the swap chain
    UINT m_flags;    //!< flags the swap chain was created with, which its buffers must keep when resized
    std::unique_ptr<void, HandleCloser> m_frame_latency_waitable_object;    //!< signalled while the presentation queue has room
    mutable bool m_swapChainIsIdle{ false };    //!< determines if swap chain is in idle state
    mutable bool m_device_lost{ false };    //!< set once the device has been removed or reset
};


template<> class SwapChainAttorney<HwAdapter>
{
    friend class HwAdapter;

private:
    static SwapChain makeSwapChain(ComPtr<IDXGIFactory6> const& dxgi_factory,
        d3d12::Device& device,
        d3d12::CommandQueue const& default_command_queue,
        HWND hwnd,
        math::Vector2u const& client_area_size,
        SwapChainDescriptor const& desc)
    {
        return SwapChain{ dxgi_factory, device, default_command_queue, hwnd, client_area_size, desc };
    }
};



}

#endif