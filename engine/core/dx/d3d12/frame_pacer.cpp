#include "frame_pacer.h"

#include "frame_progress_tracker.h"

using namespace lexgine::core::dx::d3d12;


FramePacer::FramePacer(FrameProgressTracker const& frame_progress_tracker, uint16_t max_frames_in_flight)
    : m_frame_progress_tracker{ frame_progress_tracker }
    , m_max_frames_in_flight{ max_frames_in_flight }
{

}

void FramePacer::waitForFrameSlot() const
{
    uint64_t scheduled_frames_count = m_frame_progress_tracker.scheduledFramesCount();
    uint64_t completed_frames_count = m_frame_progress_tracker.completedFramesCount();

    if (scheduled_frames_count - completed_frames_count >= m_max_frames_in_flight)
    {
        m_frame_progress_tracker.waitForFrameCompletion(completed_frames_count);
    }
}
