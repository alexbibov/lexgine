#include "frame_progress_tracker.h"

using namespace lexgine::core::dx::d3d12;

FrameProgressTracker::FrameProgressTracker(Device& device)
    : m_cpu_wall_signal{ device }
    , m_gpu_wall_signal{ device }
{
}

uint64_t FrameProgressTracker::scheduledFramesCount() const
{
    return m_cpu_wall_signal.lastValueSignaled();
}

uint64_t FrameProgressTracker::completedFramesCount() const
{
    return m_gpu_wall_signal.lastValueSignaled();
}

uint64_t FrameProgressTracker::currentFrameIndex() const
{
    return m_cpu_wall_signal.nextValueOfSignal() - 1;
}

void FrameProgressTracker::waitForFrameCompletion(uint64_t frame_index) const
{
    m_gpu_wall_signal.waitUntilValue(frame_index + 1);
}

void FrameProgressTracker::waitForFrameCompletion(uint64_t frame_index, uint32_t timeout_in_milliseconds) const
{
    m_gpu_wall_signal.waitUntilValue(frame_index + 1, timeout_in_milliseconds);
}
