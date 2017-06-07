#ifndef LEXGINE_CORE_DX_D3D12_FENCE_H

#include <d3d12.h>
#include <wrl.h>

#include "../../entity.h"
#include "../../class_names.h"
#include "../../../osinteraction/windows/fence_event.h"


using namespace Microsoft::WRL;

namespace lexgine {namespace core {namespace dx {namespace d3d12 {

//! Thin wrapper over Direct3D 12 fence
class Fence final : public NamedEntity<class_names::D3D12Fence>
{
    friend class Device;
    friend class CommandQueue;
public:

    Device& device() const;    //! returns device that was used to create the fence
    ComPtr<ID3D12Fence> native() const;    //! returns native ID3D12Fence interface for the fence

    void setEvent(uint64_t number_of_crosses, osinteraction::windows::FenceEvent const& event) const;    //! sets event that will be fired when the fence has been crossed the given number of times
    bool isShared() const;    //! returns 'true' if the fence is shared between multiple GPUs
    void cross() const;    //! crosses the fence on the CPU-side


    Fence(Fence const&) = delete;
    Fence(Fence&&) = default;

private:
    Fence(Device& device, bool is_shared);

    ComPtr<ID3D12Fence> m_fence;    //!< reference to the native Direct3D 12 interface
    Device& m_device;    //!< reference to the device interface that was used to create the fence
    bool m_is_shared;    //!< 'true' if the fence is shared between devices
    mutable uint64_t m_target_value;    //!< the next value the fence has to reach
};

}}}}

#define LEXGINE_CORE_DX_D3D12_FENCE_H
#endif


