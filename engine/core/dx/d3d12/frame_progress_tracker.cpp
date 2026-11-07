#include <chrono>

#include "frame_progress_tracker.h"

#include "engine/core/dx/d3d12/device.h"

#include "engine/core/concurrency/thread_affinity.h"

using namespace lexgine::core::dx::d3d12;

namespace
{
    uint64_t signalQueueWall(Signal& wall, CommandQueue const& queue)
    {
        lexgine::core::concurrency::assertOnRenderThread();
        uint64_t signal_index = wall.nextValueOfSignal() - 1;
        wall.signalFromGPU(queue);
        return signal_index;
    }

    void waitUntilReached(Signal const& wall, uint64_t value)
    {
        if (wall.lastValueSignaled() < value) wall.waitUntilValue(value);
    }

    bool waitUntilReached(Signal const& wall, uint64_t value, uint32_t timeout_in_milliseconds)
    {
        return wall.lastValueSignaled() >= value || wall.waitUntilValue(value, timeout_in_milliseconds);
    }
}  // namespace

FrameProgressTracker::FrameProgressTracker(Device& device)
    : m_device{ device }
    , m_cpu_wall_signal{ device }
    , m_gpu_wall_signal{ device }
    , m_compute_wall_signal{ device }
    , m_copy_wall_signal{ device }
    , m_current_recorded_frame_index{ 0 }
{
    m_cpu_wall_signal.setStringName("CPU wall");
    m_gpu_wall_signal.setStringName("GPU wall");
    m_compute_wall_signal.setStringName("Compute wall");
    m_copy_wall_signal.setStringName("Copy wall");
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
    waitUntilReached(m_gpu_wall_signal, m_cpu_wall_signal.lastValueSignaled());
    waitUntilReached(m_compute_wall_signal, scheduledComputeQueueSignalsCount());
    waitUntilReached(m_copy_wall_signal, scheduledCopyQueueSignalsCount());
}

bool FrameProgressTracker::synchronize(uint32_t timeout_in_milliseconds) const
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{ timeout_in_milliseconds };
    auto remaining_milliseconds = [&deadline]()
    {
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        return static_cast<uint32_t>((std::max)(remaining.count(), decltype(remaining)::rep{ 0 }));
    };

    return waitUntilReached(m_gpu_wall_signal, m_cpu_wall_signal.lastValueSignaled(), remaining_milliseconds())
        && waitUntilReached(m_compute_wall_signal, scheduledComputeQueueSignalsCount(), remaining_milliseconds())
        && waitUntilReached(m_copy_wall_signal, scheduledCopyQueueSignalsCount(), remaining_milliseconds());
}

uint64_t FrameProgressTracker::signalComputeQueue()
{
    return signalQueueWall(m_compute_wall_signal, m_device.asyncCommandQueue());
}

uint64_t FrameProgressTracker::scheduledComputeQueueSignalsCount() const
{
    return m_compute_wall_signal.nextValueOfSignal() - 1;
}

uint64_t FrameProgressTracker::completedComputeQueueSignalsCount() const
{
    return m_compute_wall_signal.lastValueSignaled();
}

void FrameProgressTracker::waitForComputeQueueSignal(uint64_t signal_index) const
{
    m_compute_wall_signal.waitUntilValue(signal_index + 1);
}

bool FrameProgressTracker::waitForComputeQueueSignal(uint64_t signal_index, uint32_t timeout_in_milliseconds) const
{
    return m_compute_wall_signal.waitUntilValue(signal_index + 1, timeout_in_milliseconds);
}

void FrameProgressTracker::waitForComputeQueueSignalOnGPU(CommandQueue const& waiting_queue, uint64_t signal_index)
{
    m_compute_wall_signal.waitOnGPUQueue(waiting_queue, signal_index + 1);
}

uint64_t FrameProgressTracker::signalCopyQueue()
{
    return signalQueueWall(m_copy_wall_signal, m_device.copyCommandQueue());
}

uint64_t FrameProgressTracker::scheduledCopyQueueSignalsCount() const
{
    return m_copy_wall_signal.nextValueOfSignal() - 1;
}

uint64_t FrameProgressTracker::completedCopyQueueSignalsCount() const
{
    return m_copy_wall_signal.lastValueSignaled();
}

void FrameProgressTracker::waitForCopyQueueSignal(uint64_t signal_index) const
{
    m_copy_wall_signal.waitUntilValue(signal_index + 1);
}

bool FrameProgressTracker::waitForCopyQueueSignal(uint64_t signal_index, uint32_t timeout_in_milliseconds) const
{
    return m_copy_wall_signal.waitUntilValue(signal_index + 1, timeout_in_milliseconds);
}

void FrameProgressTracker::waitForCopyQueueSignalOnGPU(CommandQueue const& waiting_queue, uint64_t signal_index)
{
    m_copy_wall_signal.waitOnGPUQueue(waiting_queue, signal_index + 1);
}
