#ifndef LEXGINE_CORE_DX_D3D12_FRAME_PACER_H
#define LEXGINE_CORE_DX_D3D12_FRAME_PACER_H

#include <cstdint>

#include "lexgine_core_dx_d3d12_fwd.h"


namespace lexgine::core::dx::d3d12 {

/*! Governs how far the CPU is allowed to run ahead of the GPU. The budget is expressed in frames and is
 enforced against the progress reported by the frame progress tracker of the rendering tasks being paced.
*/
class FramePacer final
{
public:
    /*! creates a pacer enforcing a budget of @param max_frames_in_flight frames against the progress reported
     by @param frame_progress_tracker. A budget exceeding @param back_buffer_count leaves the pacing to the
     presentation engine rather than to this gate, so it is reported and reduced to the number of back buffers.
    */
    FramePacer(FrameProgressTracker const& frame_progress_tracker, uint16_t max_frames_in_flight,
        uint32_t back_buffer_count);

    //! returns the tracker reporting the progress of the frames being paced
    FrameProgressTracker const& frameProgressTracker() const { return m_frame_progress_tracker; }

    uint16_t maxFramesInFlight() const { return m_max_frames_in_flight; }    //! returns the frames in flight budget

    //! blocks until another frame may be scheduled without exceeding the frames in flight budget
    void waitForFrameSlot() const;

private:
    FrameProgressTracker const& m_frame_progress_tracker;
    uint16_t m_max_frames_in_flight;
};

}

#endif
