#include "frame_progress_tracker.h"

#include "engine/core/dx/d3d12/device.h"

#include "engine/core/concurrency/thread_affinity.h"

using namespace lexgine::core::dx::d3d12;

FrameProgressTracker::FrameProgressTracker(Device& device)
    : m_device{ device }
    , m_cpu_wall_signal{ device }
    , m_gpu_wall_signal{ device }
    , m_current_recorded_frame_index{ 0 }
{
    m_cpu_wall_signal.setStringName("CPU wall");
    m_gpu_wall_signal.setStringName("GPU wall");
}

uint64_t FrameProgressTracker::markFrameBegin()
{
    concurrency::assertOnRenderThread();
    return (m_current_recorded_frame_index = m_cpu_wall_signal.lastValueSignaled());
}

uint64_t FrameProgressTracker::markFrameEnd()
{
    concurrency::assertOnRenderThread();
    assert(m_current_recorded_frame_index == m_cpu_wall_signal.lastValueSignaled());
    m_cpu_wall_signal.signalFromCPU();
    m_gpu_wall_signal.signalFromGPU(m_device.defaultCommandQueue());
    return m_current_recorded_frame_index;
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
    return m_current_recorded_frame_index;
}

void FrameProgressTracker::waitForFrameCompletion(uint64_t frame_index) const
{
    m_gpu_wall_signal.waitUntilValue(frame_index + 1);
}

bool FrameProgressTracker::waitForFrameCompletion(uint64_t frame_index, uint32_t timeout_in_milliseconds) const
{
    return m_gpu_wall_signal.waitUntilValue(frame_index + 1, timeout_in_milliseconds);
}


void FrameProgressTracker::synchronize() const
{
    m_gpu_wall_signal.waitUntilValue(m_cpu_wall_signal.lastValueSignaled());
}

void FrameProgressTracker::synchronize(uint32_t timeout_in_milliseconds) const
{
    m_gpu_wall_signal.waitUntilValue(m_cpu_wall_signal.lastValueSignaled(), timeout_in_milliseconds);
}
