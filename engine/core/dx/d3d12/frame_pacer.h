#ifndef LEXGINE_CORE_DX_D3D12_FRAME_PACER_H
#define LEXGINE_CORE_DX_D3D12_FRAME_PACER_H

#include <chrono>
#include <cstdint>

#include "engine/core/dx/dxgi/lexgine_core_dx_dxgi_fwd.h"

#include "lexgine_core_dx_d3d12_fwd.h"
#include "frame_progress_tracker.h"


namespace lexgine::core::dx::d3d12 {

/*! Governs how far the CPU is allowed to run ahead of the GPU and of the display. The budget is expressed in
 frames and is enforced against the progress reported by the frame progress tracker of the rendering tasks being
 paced; the frames are also held back while the presentation queue of the swap chain is full.
*/
class FramePacer final
{
    static constexpr std::chrono::milliseconds c_frame_latency_timeout{ 1000 };

public:
    /*! creates a pacer enforcing a budget of @param max_frames_in_flight frames against the progress reported
     by @param frame_progress_tracker and the frame latency of @param swap_chain. A budget exceeding the number of
     back buffers of the swap chain leaves the pacing to the presentation engine rather than to this gate, so it is
     reported and reduced to the number of back buffers.
    */
    FramePacer(FrameProgressTracker& frame_progress_tracker, dxgi::SwapChain const& swap_chain,
        uint16_t max_frames_in_flight);

    //! returns the tracker reporting the progress of the frames being paced
    FrameProgressTracker const& frameProgressTracker() const { return m_frame_progress_tracker; }

    uint16_t maxFramesInFlight() const { return m_max_frames_in_flight; }    //! returns the frames in flight budget

    /*! blocks until the presentation queue has room for another frame and another frame may be scheduled without
     exceeding the frames in flight budget
    */
    void waitForFrameSlot() const;

    uint64_t markFrameBegin() { return m_frame_progress_tracker.markFrameBegin(); }
    uint64_t markFrameEnd() { return m_frame_progress_tracker.markFrameEnd(); }

private:
    FrameProgressTracker& m_frame_progress_tracker;
    dxgi::SwapChain const& m_swap_chain;
    uint16_t m_max_frames_in_flight;
};

}

#endif
