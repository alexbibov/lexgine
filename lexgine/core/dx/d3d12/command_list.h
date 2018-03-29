#ifndef LEXGINE_CORE_DX_D3D12_COMMAND_LIST_H

#include <wrl.h>
#include <d3d12.h>

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "pipeline_state.h"

using namespace Microsoft::WRL;

namespace lexgine {namespace core {namespace dx {namespace d3d12 {


//! Enumerates types of command lists
enum class CommandListType
{
    direct = D3D12_COMMAND_LIST_TYPE_DIRECT,    //!< command list is intended for execution of graphics commands
    bundle = D3D12_COMMAND_LIST_TYPE_BUNDLE,    //!< command list is a command bundle (see Direct3D docs for details)
    compute = D3D12_COMMAND_LIST_TYPE_COMPUTE,    //!< command list is intended for execution of compute commands
    copy = D3D12_COMMAND_LIST_TYPE_COPY    //!< command list is intended to be used by the copy engine
};


class CommandList: public NamedEntity<class_names::D3D12CommandList>
{
    friend class CommandAllocator;    // only command allocators are capable of creating command lists
public:
    CommandAllocator& commandAllocator() const;    //! returns command allocator that owns this command list
    uint32_t getNodeMask() const;    //! returns the node mask determining which node on the adapter link owns the command list
    ComPtr<ID3D12GraphicsCommandList> native() const;    //! returns pointer to the native ID3D12GraphicsCommandList interface

    void reset(PipelineState const& initial_pipeline_state);   //! resets the command list back to its initial state as if it was just created
    void reset();    //! resets the command list back to its initial state as if it was just created and sets its initial pipeline state to dummy PSO (implemented on the driver level)

    void setStringName(std::string const& entity_string_name);	//! sets new user-friendly string name for the command list

    bool isClosed() const;    //! returns 'true' if the command list is closed for writing

    CommandListType type() const;    //! returns type of the command list

    CommandList(CommandList const&) = delete;
    CommandList(CommandList&&) = default;

private:
    CommandList(CommandAllocator& command_allocator, uint32_t node_mask, PipelineState const& initial_pipeline_state);
    CommandList(CommandAllocator& command_allocator, uint32_t node_mask);

    CommandAllocator& m_command_allocator;    //!< command allocator to which the command list belongs
    uint32_t m_node_mask;    //!< node mask determining on which node the command list resides
    ComPtr<ID3D12GraphicsCommandList> m_command_list;    //!< pointer to the native Direct3D12 command list interface
    bool m_is_closed;    //!< 'true' if the command list is closed for writing
};

}}}}

#define LEXGINE_CORE_DX_D3D12_COMMAND_LIST_H
#endif
