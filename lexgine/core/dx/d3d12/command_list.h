#ifndef LEXGINE_CORE_DX_D3D12_COMMAND_LIST_H
#define LEXGINE_CORE_DX_D3D12_COMMAND_LIST_H

#include <wrl.h>
#include <d3d12.h>

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"

#include "lexgine_core_dx_d3d12_fwd.h"
#include "command_allocator_ring.h"
#include "fence.h"
#include "lexgine/osinteraction/windows/fence_event.h"

using namespace Microsoft::WRL;

namespace lexgine {namespace core {namespace dx {namespace d3d12 {


template<typename T> class CommandListAttorney;


struct Signal
{
    Fence fence;
    osinteraction::windows::FenceEvent event;

    Signal(Device& device, FenceSharing sync_mode = FenceSharing::none);
};


class CommandList: public NamedEntity<class_names::D3D12CommandList>
{
    friend class CommandListAttorney<Device>;
    friend class CommandListAttorney<CommandQueue>;

public:
    uint32_t getNodeMask() const;    //! returns the node mask determining which node on the adapter link owns the command list
    ComPtr<ID3D12GraphicsCommandList> native() const;    //! returns pointer to the native ID3D12GraphicsCommandList interface

    void reset(PipelineState const* initial_pipeline_state = nullptr);   //! resets the command list back to its initial state as if it was just created

    void setStringName(std::string const& entity_string_name) override;	//! sets new user-friendly string name for the command list

    bool isClosed() const;    //! returns 'true' if the command list is closed for writing

    CommandType commandType() const;    //! returns type of the command list

    Device& device() const;    //! returns reference for the device that created this command list

public:
    CommandList(CommandList const&) = delete;
    CommandList(CommandList&&) = default;

private:
    CommandList(Device& device, CommandType command_workload_type, uint32_t node_mask, 
        FenceSharing command_list_sync_mode = FenceSharing::none, PipelineState const* initial_pipeline_state = nullptr);

    void defineSignalingCommandList(CommandList const& signaling_command_list);
    Signal const* getJobCompletionSignalPtr() const;

private:
    CommandAllocatorRing m_allocator_ring;    //!< command allocator to which the command list belongs
    uint32_t m_node_mask;    //!< node mask determining on which node the command list resides
    ComPtr<ID3D12GraphicsCommandList> m_command_list;    //!< pointer to the native Direct3D12 command list interface
    bool m_is_closed;    //!< 'true' if the command list is closed for writing
    Signal m_signal;    //!< signal, which fires when the batch, as part of which this command list is submitted for execution is completed
};


template<> class CommandListAttorney<Device>
{
    friend class Device;

private:
    static CommandList makeCommandList(Device& device, CommandType command_workload_type, 
        uint32_t node_mask, FenceSharing command_list_sync_mode = FenceSharing::none,
        PipelineState const* initial_pipeline_state = nullptr)
    {
        return CommandList{ device, command_workload_type, node_mask, command_list_sync_mode, initial_pipeline_state };
    }
};

template<> class CommandListAttorney<CommandQueue>
{
    friend class CommandQueue;

private:
    static void defineSignalingCommandListForTargetCommandList(CommandList& target_command_list, CommandList const& signaling_command_list)
    {
        target_command_list.defineSignalingCommandList(signaling_command_list);
    }

    static Signal const* getJobCompletionSignalPtrForCommandList(CommandList const& parent_command_list)
    {
        return parent_command_list.getJobCompletionSignalPtr();
    }
};


}}}}


#endif
