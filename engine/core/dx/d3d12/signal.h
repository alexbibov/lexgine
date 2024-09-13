#ifndef LEXGINE_CORE_DX_D3D12_SIGNAL_H
#define LEXGINE_CORE_DX_D3D12_SIGNAL_H

#include "fence.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "engine/osinteraction/windows/fence_event.h"
#include "engine/core/misc/optional.h"

namespace lexgine::core::dx::d3d12 {

//! Simple wrapper implementing coupling between fence and event APIs
class Signal final
{
public:
    Signal(Device& device, FenceSharing sync_mode = FenceSharing::none);

    Device& device() const;    //! Returns reference for device, which has been used to create this signal

    FenceSharing fenceSharingMode() const;    //! Sharing mode of the underlying fence object

    void signalFromCPU();    //! Signals underlying fence object from the CPU side
    void signalFromGPU(CommandQueue const& signaling_queue);    //! Signals underlying fence object from GPU side on the given GPU queue

    uint64_t lastValueRecorded() const;    //! Last value, which has been recorded by the signal, but not necessarily signaled
    uint64_t lastValueSignaled() const;    //! Last value, which has been already signaled
    uint64_t nextValueOfSignal() const;    //! Returns the next value of the signal

    void waitUntilValue(uint64_t signal_value) const;    //! Blocks the calling thread until the signal crosses the specified value

    /*!
     Blocks the calling thread for at most given amount of milliseconds or until the signal crosses
     the specified value. Returns 'true' if the value has been crossed by the signal.
     Returns 'false' if the function returned due to timeout.
    */
    bool waitUntilValue(uint64_t signal_value, uint32_t milliseconds) const;

    //! Inserts wait object into the waiting_queue which remains active until this signal reaches the given signal_value
    void waitOnGPUQueue(CommandQueue const& waiting_queue, uint64_t signal_value);

private:
    Fence m_fence;
    osinteraction::windows::FenceEvent m_event;
};

}

#endif
