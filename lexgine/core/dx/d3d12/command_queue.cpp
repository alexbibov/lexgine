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

void CommandQueue::executeCommandLists(std::vector<CommandList*> const& command_lists) const
{
    auto last_cmd_list = command_lists.back();    // only the last command list in the batch signals job completion to avoid redundancy

    std::vector<ID3D12CommandList*> native_command_lists(command_lists.size());
    for (size_t i = 0U; i < command_lists.size(); ++i)
    {
        CommandListAttorney<CommandQueue>::defineSignalingCommandListForTargetCommandList(*command_lists[i], *last_cmd_list);
        native_command_lists[i] = command_lists[i]->native().Get();
    }


    m_command_queue->ExecuteCommandLists(static_cast<UINT>(native_command_lists.size()), native_command_lists.data());
    
    Signal const* job_completion_signal_ptr = CommandListAttorney<CommandQueue>::getJobCompletionSignalPtrForCommandList(*last_cmd_list);
    signal(job_completion_signal_ptr->fence);    // signal job completion
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
