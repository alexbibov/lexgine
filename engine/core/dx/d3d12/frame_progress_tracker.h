#ifndef LEXGINE_CORE_DX_D3D12_FRAME_PROGRESS_TRACKER_H
#define LEXGINE_CORE_DX_D3D12_FRAME_PROGRESS_TRACKER_H

#include "lexgine_core_dx_d3d12_fwd.h"
#include "signal.h"

#include "pix_support.h"

namespace lexgine::core::dx::d3d12 {

template<typename T> class FrameProgressTrackerAttorney;

class FrameProgressTracker
{
    friend class FrameProgressTrackerAttorney<QueryCache>;

public:
    FrameProgressTracker(Device& device);

    Device& device() { return m_device; }
    Device const& device() const { return m_device; }

    uint64_t markFrameBegin();  //! marks beginning of new frame recording and returns the index of that frame
    uint64_t markFrameEnd();  //! marks the end of the frame, which is currently being recorded

    uint64_t lastCompletedFrameIndex() const { return completedFramesCount() - 1; }
    uint64_t scheduledFramesCount() const;
    uint64_t completedFramesCount() const;
    uint64_t currentFrameIndex() const;

    void waitForFrameCompletion(uint64_t frame_index) const;
    bool waitForFrameCompletion(uint64_t frame_index, uint32_t timeout_in_milliseconds) const;

    void synchronize() const;
    void synchronize(uint32_t timeout_in_milliseconds) const;

private:
    Device& m_device;
    Signal m_cpu_wall_signal;
    Signal m_gpu_wall_signal;
    uint64_t m_current_recorded_frame_index;
};


//template<> class FrameProgressTrackerAttorney<QueryCache>
//{
//    friend class QueryCache;
//
//    static void signalCPUBeginFrame(FrameProgressTracker& progress_tracker)
//    {
//        PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
//            "CPU job for frame %i start", progress_tracker.currentFrameIndex());
//    }
//
//    static void signalCPUEndFrame(FrameProgressTracker& progress_tracker)
//    {
//        PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
//            "CPU job for frame %i finished", progress_tracker.currentFrameIndex());
//        progress_tracker.m_cpu_wall_signal.signalFromCPU();
//    }
//
//    static void signalGPUBeginFrame(FrameProgressTracker& progress_tracker,
//        CommandQueue const& signalling_gpu_queue)
//    {
//        PIXSetMarker(signalling_gpu_queue.native().Get(), pix_marker_colors::PixGPUGeneralJobColor,
//            "GPU job for frame %i start", progress_tracker.currentFrameIndex());
//    }
//
//    static void signalGPUEndFrame(FrameProgressTracker& progress_tracker,
//        CommandQueue const& signalling_gpu_queue)
//    {
//        PIXSetMarker(signalling_gpu_queue.native().Get(), pix_marker_colors::PixGPUGeneralJobColor,
//            "GPU job for frame %i finished", progress_tracker.currentFrameIndex());
//        progress_tracker.m_gpu_wall_signal.signalFromGPU(signalling_gpu_queue);
//    }
//};

}

#endif
