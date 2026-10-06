#include <sstream>

#include "frame_pacer.h"

#include "engine/core/misc/log.h"

#include "engine/core/concurrency/thread_affinity.h"

#include "frame_progress_tracker.h"

using namespace lexgine::core::dx::d3d12;


namespace {

uint16_t reconcileFramesInFlight(uint16_t max_frames_in_flight, uint32_t back_buffer_count)
{
    if (!max_frames_in_flight)
    {
        if (lexgine::core::misc::Log* p_logger = lexgine::core::misc::Log::retrieve())
        {
            p_logger->out("ERROR: the frames in flight budget is 0, which would stall every frame. "
                "The budget is raised to 1", lexgine::core::misc::LogMessageType::error);
        }

        return 1;
    }

    if (back_buffer_count && max_frames_in_flight > back_buffer_count)
    {
        if (lexgine::core::misc::Log* p_logger = lexgine::core::misc::Log::retrieve())
        {
            std::stringstream err{};
            err << "ERROR: the frames in flight budget (" << max_frames_in_flight
                << ") exceeds the number of back buffers (" << back_buffer_count
                << "), which leaves the frame pacing to the presentation engine. The budget is reduced to "
                << back_buffer_count;
            p_logger->out(err.str(), lexgine::core::misc::LogMessageType::error);
        }

        return static_cast<uint16_t>(back_buffer_count);
    }

    return max_frames_in_flight;
}

}  // namespace


FramePacer::FramePacer(FrameProgressTracker const& frame_progress_tracker, uint16_t max_frames_in_flight,
    uint32_t back_buffer_count)
    : m_frame_progress_tracker{ frame_progress_tracker }
    , m_max_frames_in_flight{ reconcileFramesInFlight(max_frames_in_flight, back_buffer_count) }
{

}

void FramePacer::waitForFrameSlot() const
{
    concurrency::assertOnRenderThread();

    uint64_t scheduled_frames_count = m_frame_progress_tracker.scheduledFramesCount();
    uint64_t completed_frames_count = m_frame_progress_tracker.completedFramesCount();

    if (scheduled_frames_count - completed_frames_count >= m_max_frames_in_flight)
    {
        m_frame_progress_tracker.waitForFrameCompletion(completed_frames_count);
    }
}
