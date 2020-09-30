#ifndef LEXGINE_CORE_DX_D3D12_FENCE_H
#define LEXGINE_CORE_DX_D3D12_FENCE_H

#include <d3d12.h>
#include <wrl.h>

#include "lexgine_core_dx_d3d12_fwd.h"
#include "command_queue.h"

#include "engine/core/entity.h"
#include "engine/core/class_names.h"

#include "engine/osinteraction/windows/lexgine_osinteraction_windows_fwd.h"


using namespace Microsoft::WRL;

namespace lexgine::core::dx::d3d12 {

template<typename T> class FenceAttorney;

enum class FenceSharing
{
    none = 0,
    shared = D3D12_FENCE_FLAG_SHARED,
    shared_cross_adapter = D3D12_FENCE_FLAG_SHARED | D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER
};


//! Thin wrapper over Direct3D 12 fence
class Fence final : public NamedEntity<class_names::D3D12_Fence>
{
    friend class FenceAttorney<Device>;
    friend class FenceAttorney<CommandQueue>;

public:
    Device& device() const;    //! returns device that was used to create the fence
    ComPtr<ID3D12Fence> native() const;    //! returns native ID3D12Fence interface for the fence

    //! sets an event, which is to be fired when the fence signals the given value
    void setEvent(uint64_t signaling_value, osinteraction::windows::FenceEvent const& event) const;
    
    FenceSharing sharingMode() const;    //! returns the sharing mode of the fence
    void signalFromCPU();    //! signals the fence on the CPU side

    uint64_t lastValueSignaled() const;    //! retrieves the last value that the fence has ALREADY signaled
    uint64_t nextValueOfSignal() const;    //! the next value to be used for the fence's signal

    Fence(Fence const&) = delete;
    Fence(Fence&&) = default;

private:
    Fence(Device& device, FenceSharing sharing = FenceSharing::none);    //! only devices are allowed to create fences

private:
    ComPtr<ID3D12Fence> m_fence;    //!< reference to the native Direct3D 12 interface
    Device& m_device;    //!< reference to the device interface that was used to create the fence
    FenceSharing m_sharing_mode;    //!< sharing mode of the fence
    uint64_t mutable m_next_signal_value;    //!< the next value the fence has to reach
};

template<> class FenceAttorney<Device>
{
    friend class Device;

private:

    static Fence makeFence(Device& device, FenceSharing sharing = FenceSharing::none)
    {
        return Fence{ device, sharing };
    }
};

template<> class FenceAttorney<CommandQueue>
{
    friend class CommandQueue;

private:

    static void signalFenceFromGPU(Fence const& fence, CommandQueue const& command_queue)
    {
        LEXGINE_LOG_ERROR_IF_FAILED(
            fence,
            command_queue.native()->Signal(fence.m_fence.Get(), fence.m_next_signal_value++),
            S_OK
        );
    }
};

}

#endif


