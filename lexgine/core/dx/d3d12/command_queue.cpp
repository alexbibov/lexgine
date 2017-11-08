#include "command_queue.h"
#include "../../exception.h"

#include <functional>

using namespace lexgine::core::dx::d3d12;


CommandQueue::CommandQueue(Device& device, CommandQueueType type, uint32_t node_mask, CommandQueuePriority priority, CommandQueueFlags flags):
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

void CommandQueue::executeCommandLists(std::vector<CommandList> const & command_lists) const
{
    ID3D12CommandList** p_cmd_lists = new ID3D12CommandList*[command_lists.size()];

    for (size_t i = 0; i < command_lists.size(); ++i)
        p_cmd_lists[i] = command_lists[i].native().Get();

    m_command_queue->ExecuteCommandLists(static_cast<UINT>(command_lists.size()), p_cmd_lists);

    delete[] p_cmd_lists;
}

Device& CommandQueue::device() const
{
    return m_device;
}

ComPtr<ID3D12CommandQueue> CommandQueue::native() const
{
    return m_command_queue;
}

void CommandQueue::setStringName(std::string const & entity_string_name)
{
    Entity::setStringName(entity_string_name);
    m_command_queue->SetName(misc::asciiStringToWstring(entity_string_name).c_str());
}

void CommandQueue::signal(Fence const & fence)
{
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_command_queue->Signal(fence.native().Get(), fence.m_target_value++), 
        S_OK
    );
}

void CommandQueue::wait(Fence const & fence, uint64_t num_crosses) const
{
    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_command_queue->Wait(fence.native().Get(), num_crosses), 
        S_OK
    );
}
