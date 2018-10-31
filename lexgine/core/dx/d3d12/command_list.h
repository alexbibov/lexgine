#ifndef LEXGINE_CORE_DX_D3D12_COMMAND_LIST_H
#define LEXGINE_CORE_DX_D3D12_COMMAND_LIST_H

#include <wrl.h>
#include <d3d12.h>

#include <variant>

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/primitive_topology.h"
#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/math/lexgine_core_math_fwd.h"

#include "lexgine_core_dx_d3d12_fwd.h"
#include "command_allocator_ring.h"
#include "fence.h"
#include "signal.h"
#include "lexgine/osinteraction/windows/fence_event.h"

using namespace Microsoft::WRL;

namespace lexgine::core::dx::d3d12 {

template<typename T> class CommandListAttorney;


class TextureCopyLocation
{
    friend class CommandList;

public:
    using native_copy_location_type =
        std::variant<UINT, D3D12_PLACED_SUBRESOURCE_FOOTPRINT>;

public:
    TextureCopyLocation(Resource const& copy_location_resource, uint32_t subresource_index);

    TextureCopyLocation(Resource const& copy_location_resource, uint64_t resource_offset,
        DXGI_FORMAT resource_format, uint32_t resource_width, uint32_t resource_height, uint32_t resource_depth, 
        uint32_t resource_row_pitch);

private:
    Resource const& m_copy_location_resource_ref;
    native_copy_location_type m_native_copy_location_desc;
};


class CommandList: public NamedEntity<class_names::D3D12CommandList>
{
    friend class CommandListAttorney<Device>;
    friend class CommandListAttorney<CommandQueue>;

public:
    uint32_t getNodeMask() const;    //! returns the node mask determining which node on the adapter link owns the command list
    ComPtr<ID3D12GraphicsCommandList> native() const;    //! returns pointer to the native ID3D12GraphicsCommandList interface

    uint32_t getMaximalReusedCommandListsInFlight() const;    //! returns maximal command lists "reuses" that can be scheduled simultaneously without blocking syncs

    void reset(PipelineState const* initial_pipeline_state = nullptr);   //! resets the command list back to its initial state as if it was just created

    void close() const;

    void resourceBarrier(uint32_t num_barriers, void const* resource_barriers_data_ptr) const;    //! records resource barriers into the command list

    void drawInstanced(uint32_t vertex_count_per_instance, uint32_t instance_count,
        uint32_t start_vertex_location, uint32_t start_instance_location) const;

    void drawIndexedInstanced(uint32_t index_count_per_instance, uint32_t instance_count,
        uint32_t start_index_location, uint32_t base_vertex_location, uint32_t start_instance_location) const;

    void dispatch(uint32_t thread_group_x, uint32_t thread_group_y = 1U, uint32_t thread_group_z = 1U) const;

    void copyBufferRegion(Resource const& dst_buffer, uint64_t dst_buffer_offset,
        Resource const& src_buffer, uint64_t src_buffer_offset, uint64_t num_bytes);

    void copyTextureRegion(TextureCopyLocation const& dst, math::Vector3u const& dst_offset, 
        TextureCopyLocation const& src, math::Box const& src_box);

    void copyResource(Resource const& dst_resource, Resource const& src_resource) const;

    void resolveSubresource(Resource const& dst_resource, uint32_t dst_subresource,
        Resource const& src_resource, uint32_t src_subresource, DXGI_FORMAT format) const;

    void inputAssemblySetPrimitiveTopology(PrimitiveTopology primitive_topology) const;

    void rasterizerStateSetViewports(std::vector<Viewport const*> const& viewports) const;

    void rasterizerStateSetScissorRectangles(std::vector<math::Rectangle const*> const& rectangles) const;

    void outputMergerSetBlendFactor(math::Vector4f const& blend_factor) const;

    void outputMergerSetStencilReference(uint32_t reference_value) const;

    void setPipelineState(PipelineState const& pipeline_state) const;

    void setDescriptorHeaps(std::vector<DescriptorHeap const*> const& descriptor_heaps) const;

    void setComputeRootSignature(std::string const& cached_root_signature_friendly_name) const;

    void setGraphicsRootSignature(std::string const& cached_root_signature_friendly_name) const;

    void setStringName(std::string const& entity_string_name) override;	//! sets new user-friendly string name for the command list

    CommandType commandType() const;    //! returns type of the command list

    Device& device() const;    //! returns reference for the device that created this command list

public:
    CommandList(CommandList const&) = delete;
    CommandList(CommandList&&) = default;

private:
    CommandList(Device& device, CommandType command_workload_type, uint32_t node_mask, 
        FenceSharing command_list_sync_mode = FenceSharing::none, PipelineState const* initial_pipeline_state = nullptr);

    void defineSignalingCommandList(CommandList const& signaling_command_list);
    Signal const* getJobCompletionSignalPtr() const;

private:
    CommandAllocatorRing m_allocator_ring;    //!< command allocator to which the command list belongs
    uint32_t m_node_mask;    //!< node mask determining on which node the command list resides
    ComPtr<ID3D12GraphicsCommandList> m_command_list;    //!< pointer to the native Direct3D12 command list interface
    Signal m_signal;    //!< signal, which fires when the batch, as part of which this command list is submitted for execution is completed
};


template<> class CommandListAttorney<Device>
{
    friend class Device;

private:
    static CommandList makeCommandList(Device& device, CommandType command_workload_type, 
        uint32_t node_mask, FenceSharing command_list_sync_mode = FenceSharing::none,
        PipelineState const* initial_pipeline_state = nullptr)
    {
        return CommandList{ device, command_workload_type, node_mask, command_list_sync_mode, initial_pipeline_state };
    }
};

template<> class CommandListAttorney<CommandQueue>
{
    friend class CommandQueue;

private:
    static void defineSignalingCommandListForTargetCommandList(CommandList& target_command_list, CommandList const& signaling_command_list)
    {
        target_command_list.defineSignalingCommandList(signaling_command_list);
    }

    static Signal const* getJobCompletionSignalPtrForCommandList(CommandList const& parent_command_list)
    {
        return parent_command_list.getJobCompletionSignalPtr();
    }
};


}


#endif
