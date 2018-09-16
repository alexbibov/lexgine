#include "command_list.h"
#include "device.h"
#include "pipeline_state.h"

#include "lexgine/core/exception.h"

using namespace lexgine::core::dx::d3d12;


Signal::Signal(Device& device, FenceSharing sync_mode/* = FenceSharing::none*/) :
    fence{ device.createFence(sync_mode) },
    event { true }
{

}

uint32_t CommandList::getNodeMask() const
{
    return m_node_mask;
}

ComPtr<ID3D12GraphicsCommandList> CommandList::native() const
{
    return m_command_list;
}

void CommandList::reset(PipelineState const* initial_pipeline_state)
{
    ComPtr<ID3D12CommandAllocator> allocator =
        CommandAllocatorRingAttorney<CommandList>::spinCommandAllocatorRing(m_allocator_ring);

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_command_list->Reset(allocator.Get(), 
            initial_pipeline_state ? initial_pipeline_state->native().Get() : NULL),
        S_OK);
    m_is_closed = false;
}

void CommandList::setStringName(std::string const& entity_string_name)
{
    Entity::setStringName(entity_string_name);

    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_command_list->SetName(misc::asciiStringToWstring(entity_string_name).c_str()),
        S_OK);
}

bool CommandList::isClosed() const
{
    return m_is_closed;
}

CommandType CommandList::commandType() const
{
    return m_allocator_ring.commandType();
}

Device& CommandList::device() const
{
    return m_allocator_ring.device();
}

CommandList::CommandList(Device& device, 
    CommandType command_workload_type, uint32_t node_mask, 
    FenceSharing command_list_sync_mode/* = FenceSharing::none*/, 
    PipelineState const* initial_pipeline_state/* = nullptr */):
    m_allocator_ring{ CommandAllocatorRingAttorney<CommandList>::makeCommandAllocatorRing(device, command_workload_type) },
    m_node_mask{ node_mask },
    m_is_closed{ true },
    m_signal{ device, command_list_sync_mode }
{
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        device.native()->CreateCommandList(node_mask, static_cast<D3D12_COMMAND_LIST_TYPE>(command_workload_type),
            CommandAllocatorRingAttorney<CommandList>::getCurrentAllocatorInAllocatorRing(m_allocator_ring).Get(),
            initial_pipeline_state ? initial_pipeline_state->native().Get() : NULL, 
            IID_PPV_ARGS(&m_command_list)),
        S_OK);

    LEXGINE_THROW_ERROR_IF_FAILED(
        this, 
        m_command_list->Close(), 
        S_OK);
}

void CommandList::defineSignalingCommandList(CommandList const& signaling_command_list)
{
    CommandAllocatorRingAttorney<CommandList>::attachSignalToCommandAllocatorRing(m_allocator_ring, signaling_command_list.m_signal);
}

Signal const* CommandList::getJobCompletionSignalPtr() const
{
    return CommandAllocatorRingAttorney<CommandList>::getCurrentJobCompletionSignalPtrFromCommandAllocatorRing(m_allocator_ring);
}
