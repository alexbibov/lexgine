#include "fence.h"
#include "device.h"

using namespace lexgine::core::dx::d3d12;

Device& Fence::device() const
{
    return m_device;
}

ComPtr<ID3D12Fence> lexgine::core::dx::d3d12::Fence::native() const
{
    return m_fence;
}

void Fence::setEvent(uint64_t number_of_crosses, lexgine::osinteraction::windows::FenceEvent const& event) const
{
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_fence->SetEventOnCompletion(number_of_crosses, event.native()),
        S_OK
    );
}

bool Fence::isShared() const
{
    return m_is_shared;
}

void Fence::cross() const
{
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_fence->Signal(m_target_value++),
        S_OK
    );
}

Fence::Fence(Device& device, bool is_shared) :
    m_device{ device },
    m_is_shared{ is_shared },
    m_target_value{ 1 }
{
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_device.native()->CreateFence(0, is_shared ? D3D12_FENCE_FLAG_SHARED | D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER : D3D12_FENCE_FLAG_NONE , IID_PPV_ARGS(&m_fence)),
        S_OK
    );
}
