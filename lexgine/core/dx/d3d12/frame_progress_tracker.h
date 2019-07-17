#ifndef LEXGINE_CORE_DX_D3D12_FRAME_PROGRESS_TRACKER_H
#define LEXGINE_CORE_DX_D3D12_FRAME_PROGRESS_TRACKER_H

#include "lexgine_core_dx_d3d12_fwd.h"
#include "signal.h"
#include "lexgine/core/dx/d3d12/tasks/rendering_tasks/profiler.h"

#include "pix_support.h"

namespace lexgine::core::dx::d3d12 {

template<typename T> class FrameProgressTrackerAttorney;

class FrameProgressTracker
{
    friend class FrameProgressTrackerAttorney<RenderingTasks>;

public:
    FrameProgressTracker(Device& device);

    uint64_t lastScheduledFrameIndex() const { return scheduledFramesCount() - 1; }
    uint64_t lastCompletedFrameIndex() const { return completedFramesCount() - 1; }
    uint64_t scheduledFramesCount() const;
    uint64_t completedFramesCount() const;
    uint64_t currentFrameIndex() const;

    void waitForFrameCompletion(uint64_t frame_index) const;
    void waitForFrameCompletion(uint64_t frame_index, uint32_t timeout_in_milliseconds) const;
   
    void synchronize() const { waitForFrameCompletion(lastScheduledFrameIndex()); }
    void synchronize(uint32_t timeout_in_milliseconds) const { waitForFrameCompletion(lastScheduledFrameIndex(), timeout_in_milliseconds); }

private:
    Signal m_cpu_wall_signal;
    Signal m_gpu_wall_signal;
};


template<> class FrameProgressTrackerAttorney<RenderingTasks>
{
    friend class RenderingTasks;

    static void signalCPUBeginFrame(FrameProgressTracker& progress_tracker)
    {
        PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
            "CPU job for frame %i start", progress_tracker.currentFrameIndex());
    }

    static void signalCPUEndFrame(FrameProgressTracker& progress_tracker)
    {
        PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
            "CPU job for frame %i finished", progress_tracker.currentFrameIndex());
        progress_tracker.m_cpu_wall_signal.signalFromCPU();
    }

    static void signalGPUBeginFrame(FrameProgressTracker& progress_tracker,
        CommandQueue const& signalling_gpu_queue)
    {
        PIXSetMarker(signalling_gpu_queue.native().Get(), pix_marker_colors::PixGPUGeneralJobColor,
            "GPU job for frame %i start", progress_tracker.currentFrameIndex());
    }

    static void signalGPUEndFrame(FrameProgressTracker& progress_tracker, 
        CommandQueue const& signalling_gpu_queue)
    {
        PIXSetMarker(signalling_gpu_queue.native().Get(), pix_marker_colors::PixGPUGeneralJobColor,
            "GPU job for frame %i finished", progress_tracker.currentFrameIndex());
        progress_tracker.m_gpu_wall_signal.signalFromGPU(signalling_gpu_queue);
    }
};

}

#endif
