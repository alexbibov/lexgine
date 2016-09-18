#ifndef LEXGINE_CORE_DX_D3D12_COMMAND_ALLOCATOR_H

#include <d3d12.h>
#include <wrl.h>

#include "device.h"
#include "command_list.h"
#include "pipeline_state.h"

using namespace Microsoft::WRL;

namespace lexgine {namespace core {namespace dx {namespace d3d12 {

//! Thin wrapper over ID3D12CommandAllocator interface
class CommandAllocator : public NamedEntity<class_names::D3D12CommandAllocator>
{
public:
    CommandAllocator(Device& device, CommandListType command_list_type);

    CommandAllocator(CommandAllocator const&) = delete;
    CommandAllocator(CommandAllocator&&) = default;

    void reset();    //! resets command allocator. It is up to the other parts of the engine to make sure that there is no GPU tasks running from within the command lists allocated by this allocator

    ComPtr<ID3D12CommandAllocator> native() const;    //! returns pointer to the native ID3D12CommandAllocator interface

    CommandList allocateCommandList(uint32_t node_mask, PipelineState const& initial_pipeline_state);    //! allocates new command list using this allocator
    CommandList allocateCommandList(uint32_t node_mask = 0);    //! allocates new command list using this allocator and sets its initial pipeline state to dummy PSO (implemented on driver level)

    CommandListType getCommandListType() const;    //! retrieves the type of command lists that could be recorded to this command allocator

    Device& device() const;   //! retrieves device interface that was used to create the command list allocator

    void setStringName(std::string const& entity_string_name);	//! sets new user-friendly string name for the command allocator

private:
    Device& m_device;    //! device that was used to create the command allocator
    CommandListType m_command_list_type;    //! type of the command lists that can be allocated by this allocator object
    ComPtr<ID3D12CommandAllocator> m_command_allocator;    //!< encapsulated pointer to ID3D12CommandAllocator interface
};

}}}}

#define LEXGINE_CORE_DX_D3D12_COMMAND_ALLOCATOR_H
#endif
