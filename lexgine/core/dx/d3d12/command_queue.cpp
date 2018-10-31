#include "command_queue.h"
#include "device.h"
#include "command_list.h"

#include "lexgine/core/exception.h"

#include <functional>

using namespace lexgine::core::dx::d3d12;


CommandQueue::CommandQueue(Device& device, WorkloadType type, uint32_t node_mask, CommandQueuePriority priority, CommandQueueFlags flags):
    m_type{ type },
    m_priority{ priority },
    m_flags{ flags },
    m_node_mask{ node_mask },
    m_device{ device }
{
    D3D12_COMMAND_QUEUE_DESC desc;
    desc.Type = static_cast<D3D12_COMMAND_LIST_TYPE>(type);
    desc.Priority = static_cast<INT>(priority);
    desc.Flags = static_cast<D3D12_COMMAND_QUEUE_FLAGS>(flags.getValue());
    desc.NodeMask = node_mask;

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        device.native()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_command_queue)), 
        S_OK
    );
}

void CommandQueue::executeCommandLists(CommandList* command_list_array, size_t num_command_lists) const
{
    auto& last_cmd_list = command_list_array[num_command_lists - 1];    // only the last command list in the batch signals job completion to avoid redundancy

    std::vector<ID3D12CommandList*> native_command_lists(num_command_lists);
    for (size_t i = 0U; i < num_command_lists; ++i)
    {
        CommandListAttorney<CommandQueue>::defineSignalingCommandListForTargetCommandList(command_list_array[i], last_cmd_list);
        native_command_lists[i] = command_list_array[i].native().Get();
    }

    m_command_queue->ExecuteCommandLists(static_cast<UINT>(num_command_lists), native_command_lists.data());
    
    Signal const* job_completion_signal_ptr = CommandListAttorney<CommandQueue>::getJobCompletionSignalPtrForCommandList(last_cmd_list);
    job_completion_signal_ptr->signalFromGPU(*this);    // signal job completion
}

void CommandQueue::executeCommandList(CommandList& command_list) const
{
    CommandListAttorney<CommandQueue>::defineSignalingCommandListForTargetCommandList(command_list, command_list);

    ID3D12CommandList* native_command_list = command_list.native().Get();
    m_command_queue->ExecuteCommandLists(1U, &native_command_list);

    Signal const* job_completion_signal_ptr = CommandListAttorney<CommandQueue>::getJobCompletionSignalPtrForCommandList(command_list);
    job_completion_signal_ptr->signalFromGPU(*this);    // signal job completion
}

Device& CommandQueue::device() const
{
    return m_device;
}

ComPtr<ID3D12CommandQueue> CommandQueue::native() const
{
    return m_command_queue;
}

void CommandQueue::setStringName(std::string const& entity_string_name)
{
    Entity::setStringName(entity_string_name);
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_command_queue->SetName(misc::asciiStringToWstring(entity_string_name).c_str()),
        S_OK
    );
}

void CommandQueue::signal(Fence const& fence) const
{
    FenceAttorney<CommandQueue>::signalFenceFromGPU(fence, *this);
}

void CommandQueue::wait(Fence const & fence, uint64_t num_crosses) const
{
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_command_queue->Wait(fence.native().Get(), num_crosses), 
        S_OK
    );
}
