#include "command_allocator.h"
#include "../../exception.h"

using namespace lexgine::core::dx::d3d12;

CommandAllocator::CommandAllocator(Device& device, CommandListType command_list_type):
    m_device{ device },
    m_command_list_type{ command_list_type }
{
    LEXGINE_THROW_ERROR_IF_FAILED(
        this, 
        device.native()->CreateCommandAllocator(static_cast<D3D12_COMMAND_LIST_TYPE>(m_command_list_type), IID_PPV_ARGS(&m_command_allocator)), 
        S_OK
    );
}

void CommandAllocator::reset()
{
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_command_allocator->Reset(), 
        S_OK
    );
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
    LEXGINE_THROW_ERROR_IF_FAILED(this,
        m_command_allocator->SetName(misc::asciiStringToWstring(entity_string_name).c_str()),
        S_OK
    );
}
