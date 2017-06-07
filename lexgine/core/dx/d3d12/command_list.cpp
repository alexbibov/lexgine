#include "command_list.h"
#include "command_allocator.h"

using namespace lexgine::core::dx::d3d12;

CommandAllocator& CommandList::commandAllocator() const
{
    return m_command_allocator;
}

uint32_t CommandList::getNodeMask() const
{
    return m_node_mask;
}

ComPtr<ID3D12GraphicsCommandList> CommandList::native() const
{
    return m_command_list;
}

void CommandList::reset(PipelineState const& initial_pipeline_state)
{
    m_command_list->Reset(m_command_allocator.native().Get(), initial_pipeline_state.native().Get());
    m_is_closed = false;
}

void CommandList::reset()
{
    m_command_list->Reset(m_command_allocator.native().Get(), NULL);
    m_is_closed = false;
}

void CommandList::setStringName(std::string const & entity_string_name)
{
    Entity::setStringName(entity_string_name);
    m_command_list->SetName(misc::asciiStringToWstring(entity_string_name).c_str());
}

bool CommandList::isClosed() const
{
    return m_is_closed;
}

CommandListType CommandList::type() const
{
    return m_command_allocator.getCommandListType();
}

CommandList::CommandList(CommandAllocator& command_allocator, uint32_t node_mask, PipelineState const& initial_pipeline_state):
    m_command_allocator{ command_allocator },
    m_node_mask{ node_mask },
    m_is_closed{ true }
{
    LEXGINE_ERROR_LOG(
        this,
        command_allocator.device().native()->CreateCommandList(node_mask, static_cast<D3D12_COMMAND_LIST_TYPE>(command_allocator.getCommandListType()),
            command_allocator.native().Get(), initial_pipeline_state.native().Get(), IID_PPV_ARGS(&m_command_list)),
        S_OK);

    m_command_list->Close();
}


CommandList::CommandList(CommandAllocator& command_allocator, uint32_t node_mask) :
    m_command_allocator{ command_allocator },
    m_node_mask{ node_mask },
    m_is_closed{ true }
{
    LEXGINE_ERROR_LOG(
        this,
        command_allocator.device().native()->CreateCommandList(node_mask, static_cast<D3D12_COMMAND_LIST_TYPE>(command_allocator.getCommandListType()),
            command_allocator.native().Get(), NULL, IID_PPV_ARGS(&m_command_list)),
        S_OK);

    m_command_list->Close();
}