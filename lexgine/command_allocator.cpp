#include "command_allocator.h"

using namespace lexgine::core::dx::d3d12;

CommandAllocator::CommandAllocator(Device& device, CommandListType command_list_type):
    m_device{ device },
    m_command_list_type{ command_list_type }
{
    LEXGINE_ERROR_LOG(logger(), device.native()->CreateCommandAllocator(static_cast<D3D12_COMMAND_LIST_TYPE>(m_command_list_type), IID_PPV_ARGS(&m_command_allocator)),
        std::bind(&CommandAllocator::raiseError, this, std::placeholders::_1), S_OK);
}

void CommandAllocator::reset()
{
    LEXGINE_ERROR_LOG(logger(), m_command_allocator->Reset(), std::bind(&CommandAllocator::raiseError, this, std::placeholders::_1), S_OK);
}

ComPtr<ID3D12CommandAllocator> CommandAllocator::native() const
{
    return m_command_allocator;
}

CommandList CommandAllocator::allocateCommandList(uint32_t node_mask, PipelineState const& initial_pipeline_state)
{
    return CommandList{ *this, node_mask, initial_pipeline_state };
}

CommandList CommandAllocator::allocateCommandList(uint32_t node_mask)
{
    return CommandList{ *this, node_mask };
}

CommandListType CommandAllocator::getCommandListType() const
{
    return m_command_list_type;
}

Device& CommandAllocator::device() const
{
    return m_device;
}

void CommandAllocator::setStringName(std::string const & entity_string_name)
{
    Entity::setStringName(entity_string_name);
    m_command_allocator->SetName(misc::ascii_string_to_wstring(entity_string_name).c_str());
}
