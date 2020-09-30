#include "fence.h"
#include "device.h"
#include "engine/core/exception.h"
#include "engine/osinteraction/windows/fence_event.h"

using namespace lexgine::core::dx::d3d12;

Device& Fence::device() const
{
    return m_device;
}

ComPtr<ID3D12Fence> lexgine::core::dx::d3d12::Fence::native() const
{
    return m_fence;
}

void Fence::setEvent(uint64_t signaling_value, lexgine::osinteraction::windows::FenceEvent const& event) const
{
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_fence->SetEventOnCompletion(signaling_value, event.native()),
        S_OK
    );
}

void Fence::signalFromCPU()
{
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_fence->Signal(m_next_signal_value++),
        S_OK
    );
}

FenceSharing Fence::sharingMode() const
{
    return m_sharing_mode;
}

uint64_t Fence::lastValueSignaled() const
{
    return static_cast<uint64_t>(m_fence->GetCompletedValue());
}

uint64_t Fence::nextValueOfSignal() const
{
    return m_next_signal_value;
}

Fence::Fence(Device& device, FenceSharing sharing/* = FenceSharing::none*/) :
    m_device{ device },
    m_sharing_mode{ sharing },
    m_next_signal_value{ 1 }
{
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_device.native()->CreateFence(0, static_cast<D3D12_FENCE_FLAGS>(sharing), IID_PPV_ARGS(&m_fence)),
        S_OK
    );
}
