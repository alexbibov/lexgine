#ifndef LEXGINE_CORE_DX_D3D12_COMMAND_QUEUE_H
#define LEXGINE_CORE_DX_D3D12_COMMAND_QUEUE_H

#include <wrl.h>
#include <d3d12.h>

#include <vector>

#include "lexgine_core_dx_d3d12_fwd.h"

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/misc/flags.h"


using namespace Microsoft::WRL;

namespace lexgine::core::dx::d3d12 {

template<typename T> class CommandQueueAttorney;

namespace __tag {
enum class tagCommandQueueFlags
{
    none = D3D12_COMMAND_QUEUE_FLAG_NONE,
    disable_gpu_timeout = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
};
}


//! Enlists types of the command queues
enum class WorkloadType
{
    direct = D3D12_COMMAND_LIST_TYPE_DIRECT,
    compute = D3D12_COMMAND_LIST_TYPE_COMPUTE,
    copy = D3D12_COMMAND_LIST_TYPE_COPY
};

//! Enlists supported command queue priorities
enum class CommandQueuePriority
{
    normal = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
    high = D3D12_COMMAND_QUEUE_PRIORITY_HIGH
};

//! Command queue behavioral flags
using CommandQueueFlags = misc::Flags<__tag::tagCommandQueueFlags>;

//! Thin wrapper over Direct3D 12 command queue
class CommandQueue final : public NamedEntity<class_names::D3D12_CommandQueue>
{
    friend class CommandQueueAttorney<Device>;

public:

    void executeCommandLists(CommandList* command_list_array, size_t num_command_lists) const;   //! sends set of command lists into the command queue for execution

    void executeCommandList(CommandList& command_list) const;    //! executes the command list on this command queue

    Device& device() const;    //! returns device that was used to create this command queue

    ComPtr<ID3D12CommandQueue> native() const;    //!< returns pointer to the native Direct3D12 command queue interface

    // copyTileMappings: implement interface API for this at certain point; when decided to support tiled resources

    // getClockCalibration: could be implemented at any time

    // getDesc: could be replaced by bunch of local getters. Can be done immediately

    // getTimeStampFrequency: could be implemented at any time

    // updateTileMappings: implement interface API for this at certain point: when decide to support tiled resources

    void setStringName(std::string const& entity_string_name) override;	//! sets new user-friendly string name for the command queue

    void signal(Fence& fence) const;    //! puts signal into the queue

    void wait(Fence const& fence, uint64_t num_crosses) const;    //! instructs the command queue to wait until the given fence is crossed num_crosses times


    CommandQueue(CommandQueue const&) = delete;
    CommandQueue(CommandQueue&&) = default;

private:
    CommandQueue(Device& device,
        WorkloadType type = WorkloadType::direct,
        uint32_t node_mask = 0,
        CommandQueuePriority priority = CommandQueuePriority::normal,
        CommandQueueFlags flags = CommandQueueFlags::enum_type::none);

private:
    ComPtr<ID3D12CommandQueue> m_command_queue;    //!< COM pointer to the native Direct3D12 command queue interface
    WorkloadType m_type;    //!< type of the command queue specified when the command queue was created
    CommandQueuePriority m_priority;    //!< priority of the command queue specified when the command queue was created
    CommandQueueFlags m_flags;    //!< flags specified during creation of the command queue
    uint32_t m_node_mask;    //!< node mask determining the physical node to which the command queue is assigned
    Device& m_device;    //!< Direct3D12 device that has created the command queue
};


template<> class CommandQueueAttorney<Device>
{
    friend class Device;

private:
    static CommandQueue makeCommandQueue(Device& device, WorkloadType command_queue_type = WorkloadType::direct,
        uint32_t node_mask = 0, CommandQueuePriority command_queue_priority = CommandQueuePriority::normal,
        CommandQueueFlags flags = CommandQueueFlags::enum_type::none)
    {
        return CommandQueue{ device, command_queue_type, node_mask, command_queue_priority, flags };
    }
};


}


#endif
