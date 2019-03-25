#include "signal.h"
#include "device.h"
#include "command_queue.h"

using namespace lexgine::core::dx::d3d12;

Signal::Signal(Device& device, FenceSharing sync_mode/* = FenceSharing::none*/) :
    m_fence{ device.createFence(sync_mode) },
    m_event{ true }
{

}

Device& Signal::device() const
{
    return m_fence.device();
}

FenceSharing Signal::fenceSharingMode() const
{
    return m_fence.sharingMode();
}

void Signal::signalFromCPU()
{
    m_fence.signalFromCPU();
}

void Signal::signalFromGPU(CommandQueue const& signaling_queue)
{
    signaling_queue.signal(m_fence);
}

uint64_t Signal::lastValueSignaled() const
{
    return m_fence.lastValueSignaled();
}

uint64_t Signal::nextValueOfSignal() const
{
    return m_fence.nextValueOfSignal();
}

void Signal::waitUntilValue(uint64_t signal_value) const
{
    m_event.reset();
    m_fence.setEvent(signal_value, m_event);
    m_event.wait();
}

bool Signal::waitUntilValue(uint64_t signal_value, uint32_t milliseconds) const
{
    m_event.reset();
    m_fence.setEvent(signal_value, m_event);
    return m_event.wait(milliseconds);
}

void Signal::waitOnGPUQueue(CommandQueue const& waiting_queue, uint64_t signal_value)
{
    waiting_queue.wait(m_fence, signal_value);
}





