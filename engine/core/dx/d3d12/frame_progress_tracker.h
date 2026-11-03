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

    //! blocks the calling thread until the GPU completes all scheduled frames and all enqueued compute and copy queue signals
    void synchronize() const;

    /*! blocks the calling thread for at most the given number of milliseconds or until the GPU completes all scheduled
     frames and all enqueued compute and copy queue signals. Returns 'true' if all of them completed and 'false' on timeout
    */
    bool synchronize(uint32_t timeout_in_milliseconds) const;

    /*! signals the wall of the asynchronous compute queue on that queue and returns the index of the signal.
     The cadence of these signals is defined by the work submitted to the queue and is independent of the frames
    */
    uint64_t signalComputeQueue();
    uint64_t scheduledComputeQueueSignalsCount() const;    //! returns the number of compute queue signals enqueued so far
    uint64_t completedComputeQueueSignalsCount() const;    //! returns the number of compute queue signals crossed by the GPU

    //! blocks the calling thread until the GPU crosses the compute queue signal with the given index
    void waitForComputeQueueSignal(uint64_t signal_index) const;

    /*! blocks the calling thread for at most the given number of milliseconds or until the GPU crosses the compute queue
     signal with the given index. Returns 'true' if the signal has been crossed and 'false' on timeout
    */
    bool waitForComputeQueueSignal(uint64_t signal_index, uint32_t timeout_in_milliseconds) const;

    //! makes @param waiting_queue wait until the GPU crosses the compute queue signal with the given index
    void waitForComputeQueueSignalOnGPU(CommandQueue const& waiting_queue, uint64_t signal_index);

    /*! signals the wall of the copy queue on that queue and returns the index of the signal. The cadence of these
     signals is defined by the work submitted to the queue and is independent of the frames
    */
    uint64_t signalCopyQueue();
    uint64_t scheduledCopyQueueSignalsCount() const;    //! returns the number of copy queue signals enqueued so far
    uint64_t completedCopyQueueSignalsCount() const;    //! returns the number of copy queue signals crossed by the GPU

    //! blocks the calling thread until the GPU crosses the copy queue signal with the given index
    void waitForCopyQueueSignal(uint64_t signal_index) const;

    /*! blocks the calling thread for at most the given number of milliseconds or until the GPU crosses the copy queue
     signal with the given index. Returns 'true' if the signal has been crossed and 'false' on timeout
    */
    bool waitForCopyQueueSignal(uint64_t signal_index, uint32_t timeout_in_milliseconds) const;

    //! makes @param waiting_queue wait until the GPU crosses the copy queue signal with the given index
    void waitForCopyQueueSignalOnGPU(CommandQueue const& waiting_queue, uint64_t signal_index);

private:
    Device& m_device;
    Signal m_cpu_wall_signal;
    Signal m_gpu_wall_signal;
    Signal m_compute_wall_signal;
    Signal m_copy_wall_signal;
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
