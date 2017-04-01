#ifndef LEXGINE_CORE_DX_D3D12_COMMAND_QUEUE_H

#include <wrl.h>
#include <d3d12.h>

#include <vector>

#include "entity.h"
#include "class_names.h"
#include "device.h"
#include "command_list.h"
#include "fence.h"

using namespace Microsoft::WRL;

namespace lexgine {namespace core {namespace dx {namespace d3d12 {


namespace __tag {
enum class tagCommandQueueFlags
{
    none = D3D12_COMMAND_QUEUE_FLAG_NONE,
    disable_gpu_timeout = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
};
}


//! Enlists types of the command queues
enum class CommandQueueType
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
class CommandQueue final : public NamedEntity<class_names::D3D12CommandQueue>
{
public:
    explicit CommandQueue(Device& device,
        CommandQueueType type = CommandQueueType::direct,
        uint32_t node_mask = 0,
        CommandQueuePriority priority = CommandQueuePriority::normal,
        CommandQueueFlags flags = CommandQueueFlags::enum_type::none);

    void executeCommandLists(std::vector<CommandList> const& command_lists) const;   //! sends set of command lists into the command queue for execution


    Device& device() const;    //! returns device that was used to create this command queue


    ComPtr<ID3D12CommandQueue> native() const;    //!< returns pointer to the native Direct3D12 command queue interface

    // copyTileMappings: implement interface API for this at certain point; when decided to support tiled resources

    // getClockCalibration: could be implemented at any time

    // getDesc: could be replaced by bunch of local getters. Can be done immediately

    // getTimeStampFrequency: could be implemented at any time

    // updateTileMappings: implement interface API for this at certain point: when decide to support tiled resources

    void setStringName(std::string const& entity_string_name);	//! sets new user-friendly string name for the command queue

    void synchronize(Fence const& fence);    //! puts synchronization fence into the command queue

    void wait(Fence const& fence, uint64_t num_crosses) const;    //! blocks the calling thread until fence is crossed at least num_crosses times


    CommandQueue(CommandQueue const&) = delete;
    CommandQueue(CommandQueue&&) = default;

private:
    ComPtr<ID3D12CommandQueue> m_command_queue;    //!< COM pointer to the native Direct3D12 command queue interface
    CommandQueueType m_type;    //!< type of the command queue specified when the command queue was created
    CommandQueuePriority m_priority;    //!< priority of the command queue specified when the command queue was created
    CommandQueueFlags m_flags;    //!< flags specified during creation of the command queue
    uint32_t m_node_mask;    //!< node mask determining the physical node to which the command queue is assigned
    Device& m_device;    //!< Direct3D12 device that has created the command queue
};

}}}}

#define LEXGINE_CORE_DX_D3D12_COMMAND_QUEUE_H
#endif
