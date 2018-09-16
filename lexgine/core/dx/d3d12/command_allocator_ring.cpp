#include "command_allocator_ring.h"

#include "lexgine/core/exception.h"

#include "device.h"
#include "fence.h"
#include "command_list.h"



using namespace lexgine::core::dx::d3d12;


CommandAllocatorRing::CommandAllocatorRing(Device& device, CommandType allocated_command_type):
    m_device{ device },
    m_allocated_command_type{ allocated_command_type },
    m_signaling_allocators(device.maxFramesInFlight()),
    m_current_index{ 0U }
{
    for (size_t i = 0; i < m_signaling_allocators.size(); ++i)
    {
        LEXGINE_THROW_ERROR_IF_FAILED(
            this,
            device.native()->CreateCommandAllocator(static_cast<D3D12_COMMAND_LIST_TYPE>(m_allocated_command_type), 
                IID_PPV_ARGS(&m_signaling_allocators[i].command_allocator)),
            S_OK
        );

        m_signaling_allocators[i].signal_ptr = nullptr;
    }
}

ComPtr<ID3D12CommandAllocator> CommandAllocatorRing::spin()
{
    ++m_current_index;

    auto& signal_desc = m_signaling_allocators[m_current_index];
    Signal const* p_signal = signal_desc.signal_ptr;
    if (p_signal
        && p_signal->fence.lastValueSignaled() < signal_desc.signal_value
        || !signal_desc.signal_value)
        p_signal->event.wait();

    LEXGINE_THROW_ERROR_IF_FAILED(this,
        signal_desc.command_allocator->Reset(),
        S_OK);

    p_signal->event.reset();

    return signal_desc.command_allocator;
}

void CommandAllocatorRing::attachSignal(Signal const& signal)
{
    auto& signal_desc = m_signaling_allocators[m_current_index];
    signal_desc.signal_ptr = &signal;
    signal_desc.signal_value = signal.fence.nextValueOfSignal();
}

ComPtr<ID3D12CommandAllocator> CommandAllocatorRing::allocator() const
{
    return m_signaling_allocators[m_current_index].command_allocator;
}

CommandType CommandAllocatorRing::commandType() const
{
    return m_allocated_command_type;
}

Device& CommandAllocatorRing::device() const
{
    return m_device;
}

void CommandAllocatorRing::setStringName(std::string const& entity_string_name)
{
    Entity::setStringName(entity_string_name);

    for (size_t i = 0U; i < m_signaling_allocators.size(); ++i)
    {
        LEXGINE_THROW_ERROR_IF_FAILED(this,
            m_signaling_allocators[i].command_allocator->
                SetName(misc::asciiStringToWstring(entity_string_name + "__RING#" + std::to_string(i)).c_str()),
            S_OK
        );
    }
}
