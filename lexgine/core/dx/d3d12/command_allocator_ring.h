#ifndef LEXGINE_CORE_DX_D3D12_COMMAND_ALLOCATOR_RING_H
#define LEXGINE_CORE_DX_D3D12_COMMAND_ALLOCATOR_RING_H

#include <d3d12.h>
#include <wrl.h>

#include "lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/entity.h"

using namespace Microsoft::WRL;

namespace lexgine::core::dx::d3d12 {

template<typename T> class CommandAllocatorRingAttorney;


//! Enumerates types of command lists
enum class CommandType
{
    direct = D3D12_COMMAND_LIST_TYPE_DIRECT,    //!< command list is intended for execution of graphics commands
    bundle = D3D12_COMMAND_LIST_TYPE_BUNDLE,    //!< command list is a command bundle (see Direct3D docs for details)
    compute = D3D12_COMMAND_LIST_TYPE_COMPUTE,    //!< command list is intended for execution of compute commands
    copy = D3D12_COMMAND_LIST_TYPE_COPY    //!< command list is intended to be used by the copy engine
};


//! Thin wrapper over ID3D12CommandAllocator interface
class CommandAllocatorRing : public NamedEntity<class_names::D3D12CommandAllocatorRing>
{
    friend class CommandAllocatorRingAttorney<CommandList>;

public:
    CommandAllocatorRing(CommandAllocatorRing const&) = delete;
    CommandAllocatorRing(CommandAllocatorRing&&) = default;

    CommandType commandType() const;    //! retrieves the type of the command lists that could be recorded to this command allocator

    Device& device() const;   //! retrieves device interface that was used to create the command list allocator

    void setStringName(std::string const& entity_string_name);    //! sets new user-friendly string name for the command allocator


private:
    CommandAllocatorRing(Device& device, CommandType allocated_command_type);
    ComPtr<ID3D12CommandAllocator> spin();    //! moves ring position to the next allocator while making sure this next allocator is available for reuse
    void attachSignal(Signal const& signal);    //! associates currently active allocator with provided signaling fence
    ComPtr<ID3D12CommandAllocator> allocator() const;    //! returns COM pointer for the currently active command allocator

private:
    struct signaling_allocator
    {
        ComPtr<ID3D12CommandAllocator> command_allocator;
        Signal const* signal_ptr;
        uint64_t signal_value;
    };

private:
    Device& m_device;    //!< device that was used to create the command allocator
    CommandType m_allocated_command_type;    //!< type of the command lists that can be allocated by this allocator object
    std::vector<signaling_allocator> m_signaling_allocators;    //!< ring of command list allocators and associated fences
    uint16_t m_current_index;    //!< current position of the ring
};

template<> class CommandAllocatorRingAttorney<CommandList>
{
    friend class CommandList;

private:
    static CommandAllocatorRing makeCommandAllocatorRing(Device& device, CommandType command_workload_type)
    {
        return CommandAllocatorRing{ device, command_workload_type };
    }

    static ComPtr<ID3D12CommandAllocator> spinCommandAllocatorRing(CommandAllocatorRing& parent_command_allocator_ring)
    {
        return parent_command_allocator_ring.spin();
    }

    static void attachSignalToCommandAllocatorRing(CommandAllocatorRing& parent_command_allocator_ring, Signal const& signal)
    {
        parent_command_allocator_ring.attachSignal(signal);
    }

    static ComPtr<ID3D12CommandAllocator> getCurrentAllocatorInAllocatorRing(CommandAllocatorRing const& parent_command_allocator_ring)
    {
        return parent_command_allocator_ring.m_signaling_allocators[parent_command_allocator_ring.m_current_index].command_allocator;
    }

    static Signal const* getCurrentJobCompletionSignalPtrFromCommandAllocatorRing(CommandAllocatorRing const& parent_command_allocator_ring)
    {
        return parent_command_allocator_ring.m_signaling_allocators[parent_command_allocator_ring.m_current_index].signal_ptr;
    }

    static uint32_t getCommandAllocatorRingCapacity(CommandAllocatorRing const& parent_command_allocator_ring)
    {
        return static_cast<uint32_t>(parent_command_allocator_ring.m_signaling_allocators.size());
    }
};

}


#endif
