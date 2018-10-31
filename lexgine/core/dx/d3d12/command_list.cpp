#include "command_list.h"
#include "device.h"
#include "pipeline_state.h"
#include "resource.h"
#include "d3d12_tools.h"

#include "lexgine/core/exception.h"
#include "lexgine/core/viewport.h"

#include "lexgine/core/math/box.h"
#include "lexgine/core/math/rectangle.h"

#include <algorithm>


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

uint32_t CommandList::getNodeMask() const
{
    return m_node_mask;
}

ComPtr<ID3D12GraphicsCommandList> CommandList::native() const
{
    return m_command_list;
}

uint32_t CommandList::getMaximalReusedCommandListsInFlight() const
{
    return CommandAllocatorRingAttorney<CommandList>::getCommandAllocatorRingCapacity(m_allocator_ring);
}

void CommandList::reset(PipelineState const* initial_pipeline_state)
{
    ComPtr<ID3D12CommandAllocator> allocator =
        CommandAllocatorRingAttorney<CommandList>::spinCommandAllocatorRing(m_allocator_ring);

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_command_list->Reset(allocator.Get(),
            initial_pipeline_state ? initial_pipeline_state->native().Get() : NULL),
        S_OK);
}

void CommandList::close() const
{
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_command_list->Close(),
        S_OK);
}

void CommandList::resourceBarrier(uint32_t num_barriers, void const* resource_barriers_data_ptr) const
{
    D3D12_RESOURCE_BARRIER const* p_barriers = static_cast<D3D12_RESOURCE_BARRIER const*>(resource_barriers_data_ptr);
    m_command_list->ResourceBarrier(num_barriers, p_barriers);
}

void CommandList::drawInstanced(uint32_t vertex_count_per_instance, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location) const
{
    m_command_list->DrawInstanced(static_cast<UINT>(vertex_count_per_instance), static_cast<UINT>(instance_count),
        static_cast<UINT>(start_vertex_location), static_cast<UINT>(start_instance_location));
}

void CommandList::drawIndexedInstanced(uint32_t index_count_per_instance, uint32_t instance_count, uint32_t start_index_location, uint32_t base_vertex_location, uint32_t start_instance_location) const
{
    m_command_list->DrawIndexedInstanced(static_cast<UINT>(index_count_per_instance), static_cast<UINT>(instance_count),
        static_cast<UINT>(start_index_location), static_cast<UINT>(base_vertex_location), static_cast<UINT>(start_instance_location));
}

void CommandList::dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z) const
{
    m_command_list->Dispatch(
        static_cast<UINT>(thread_group_x),
        static_cast<UINT>(thread_group_y),
        static_cast<UINT>(thread_group_z));
}

void CommandList::copyBufferRegion(Resource const& dst_buffer, uint64_t dst_buffer_offset, 
    Resource const& src_buffer, uint64_t src_buffer_offset, uint64_t num_bytes)
{
    m_command_list->CopyBufferRegion(dst_buffer.native().Get(), static_cast<UINT64>(dst_buffer_offset),
        src_buffer.native().Get(), static_cast<UINT64>(src_buffer_offset), static_cast<UINT64>(num_bytes));
}

void CommandList::copyTextureRegion(TextureCopyLocation const& dst, math::Vector3u const& dst_offset, 
    TextureCopyLocation const& src, math::Box const& src_box)
{
    D3D12_TEXTURE_COPY_LOCATION dst_copy_location;
    {
        dst_copy_location.pResource = dst.m_copy_location_resource_ref.native().Get();
        if (std::holds_alternative<UINT>(dst.m_native_copy_location_desc))
        {
            dst_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dst_copy_location.SubresourceIndex = 
                static_cast<UINT>(std::get<UINT>(dst.m_native_copy_location_desc));
        }
        else
        {
            dst_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            dst_copy_location.PlacedFootprint =
                static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(std::get<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(dst.m_native_copy_location_desc));
        }
    }

    D3D12_TEXTURE_COPY_LOCATION src_copy_location;
    {
        src_copy_location.pResource = src.m_copy_location_resource_ref.native().Get();
        if (std::holds_alternative<UINT>(src.m_native_copy_location_desc))
        {
            src_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            src_copy_location.SubresourceIndex =
                static_cast<UINT>(std::get<UINT>(src.m_native_copy_location_desc));
        }
        else
        {
            src_copy_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            src_copy_location.PlacedFootprint =
                static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(std::get<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(src.m_native_copy_location_desc));
        }
    }

    D3D12_BOX box;
    {
        math::Vector3u left_bottom_far{ static_cast<math::Vector3u>(src_box.center() - src_box.extents()) };
        math::Vector3u right_top_near{ static_cast<math::Vector3u>(src_box.center() + src_box.extents()) };

        box.left = static_cast<UINT>(left_bottom_far.x);
        box.bottom = static_cast<UINT>(left_bottom_far.y);
        box.back = static_cast<UINT>(left_bottom_far.z);

        box.right = static_cast<UINT>(right_top_near.x);
        box.top = static_cast<UINT>(right_top_near.y);
        box.front = static_cast<UINT>(right_top_near.z);
    }

    m_command_list->CopyTextureRegion(&dst_copy_location,
        static_cast<UINT>(dst_offset.x), static_cast<UINT>(dst_offset.y), static_cast<UINT>(dst_offset.z),
        &src_copy_location, &box);
}

void CommandList::copyResource(Resource const& dst_resource, Resource const& src_resource) const
{
    m_command_list->CopyResource(dst_resource.native().Get(), src_resource.native().Get());
}

void CommandList::resolveSubresource(Resource const& dst_resource, uint32_t dst_subresource,
    Resource const& src_resource, uint32_t src_subresource, DXGI_FORMAT format) const
{
    m_command_list->ResolveSubresource(dst_resource.native().Get(), static_cast<UINT>(dst_subresource),
        src_resource.native().Get(), static_cast<UINT>(src_subresource), format);
}

void CommandList::inputAssemblySetPrimitiveTopology(PrimitiveTopology primitive_topology) const
{
    uint8_t native_topology;
    switch (primitive_topology)
    {
    case PrimitiveTopology::point:
        native_topology = 
            misc::PrimitiveTopologyConverter<misc::EngineAPI::Direct3D12, PrimitiveTopology::point>::value();
        break;
    case PrimitiveTopology::line:
        native_topology = 
            misc::PrimitiveTopologyConverter<misc::EngineAPI::Direct3D12, PrimitiveTopology::line>::value();
        break;
    case PrimitiveTopology::triangle:
        native_topology =
            misc::PrimitiveTopologyConverter<misc::EngineAPI::Direct3D12, PrimitiveTopology::triangle>::value();
        break;
    case PrimitiveTopology::patch:
        native_topology =
            misc::PrimitiveTopologyConverter<misc::EngineAPI::Direct3D12, PrimitiveTopology::patch>::value();
        break;
    }

    m_command_list->IASetPrimitiveTopology(static_cast<D3D12_PRIMITIVE_TOPOLOGY>(native_topology));
}

void CommandList::rasterizerStateSetViewports(std::vector<Viewport const*> const& viewports) const
{
    std::vector<D3D12_VIEWPORT> native_viewports(viewports.size());
    std::transform(viewports.begin(), viewports.end(), native_viewports.begin(),
        [](Viewport const* v)
        {
            D3D12_VIEWPORT rv{};

            auto top_left_corner = v->topLeftCorner();
            rv.TopLeftX = static_cast<FLOAT>(top_left_corner.x);
            rv.TopLeftY = static_cast<FLOAT>(top_left_corner.y);

            rv.Width = static_cast<FLOAT>(v->width());
            rv.Height = static_cast<FLOAT>(v->height());

            auto depth_range = v->depthRange();
            rv.MinDepth = static_cast<FLOAT>(depth_range.x);
            rv.MaxDepth = static_cast<FLOAT>(depth_range.y);

            return rv;
        }
    );

    m_command_list->RSSetViewports(static_cast<UINT>(viewports.size()), native_viewports.data());
}

void CommandList::rasterizerStateSetScissorRectangles(std::vector<math::Rectangle const*> const& rectangles) const
{
    std::vector<D3D12_RECT> native_rects(rectangles.size());
    std::transform(rectangles.begin(), rectangles.end(), native_rects.begin(),
        [](math::Rectangle const* r)
        {
            D3D12_RECT rv{};
            
            auto ul = r->upperLeft();
            rv.left = static_cast<LONG>(ul.x);
            rv.top = static_cast<LONG>(ul.y);

            auto dims = r->size();
            rv.right = static_cast<LONG>(ul.x + dims.x);
            rv.bottom = static_cast<LONG>(ul.y - dims.y);

            return rv;
        }
    );

    m_command_list->RSSetScissorRects(static_cast<UINT>(rectangles.size()), native_rects.data());
}

void CommandList::outputMergerSetBlendFactor(math::Vector4f const& blend_factor) const
{
    m_command_list->OMSetBlendFactor(blend_factor.getDataAsArray());
}

void CommandList::outputMergerSetStencilReference(uint32_t reference_value) const
{
    m_command_list->OMSetStencilRef(static_cast<UINT>(reference_value));
}

void CommandList::setPipelineState(PipelineState const& pipeline_state) const
{
    m_command_list->SetPipelineState(pipeline_state.native().Get());
}

void CommandList::setDescriptorHeaps(std::vector<DescriptorHeap const*> const& descriptor_heaps) const
{
    std::vector<ID3D12DescriptorHeap*> native_descriptor_heaps(descriptor_heaps.size());
    std::transform(descriptor_heaps.begin(), descriptor_heaps.end(), native_descriptor_heaps.begin(),
        [](DescriptorHeap const* dh) { return dh->native().Get(); });

    m_command_list->SetDescriptorHeaps(static_cast<UINT>(descriptor_heaps.size()), native_descriptor_heaps.data());
}

void CommandList::setComputeRootSignature(std::string const& cached_root_signature_friendly_name) const
{
    auto rs = device().retrieveRootSignature(cached_root_signature_friendly_name, m_node_mask);
    if (!rs)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Cannot set compute root signature for command list \""
        + getStringName() + "\": the root signature is identified to have friendly name \"" + cached_root_signature_friendly_name
        + "\", but no root signature with this friendly name and the node mask " + std::to_string(m_node_mask)
        + " exists in the device cache. This usually means that the root signature has not been created for "
        " the node mask required by the command list attempting to set this root signature");
    }
    else
    {
        m_command_list->SetComputeRootSignature(rs.Get());
    }
}

void CommandList::setGraphicsRootSignature(std::string const& cached_root_signature_friendly_name) const
{
    auto rs = device().retrieveRootSignature(cached_root_signature_friendly_name, m_node_mask);
    if (!rs)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Cannot set graphics root signature for command list \""
            + getStringName() + "\": the root signature is identified to have friendly name \"" + cached_root_signature_friendly_name
            + "\", but no root signature with this friendly name and the node mask " + std::to_string(m_node_mask)
            + " exists in the device cache. This usually means that the root signature has not been created for "
            " the node mask required by the command list attempting to set this root signature");
    }
    else
    {
        m_command_list->SetGraphicsRootSignature(rs.Get());
    }
}


void CommandList::setStringName(std::string const& entity_string_name)
{
    Entity::setStringName(entity_string_name);

    LEXGINE_LOG_ERROR_IF_FAILED(
        this,
        m_command_list->SetName(misc::asciiStringToWstring(entity_string_name).c_str()),
        S_OK);
}

CommandType CommandList::commandType() const
{
    return m_allocator_ring.commandType();
}

Device& CommandList::device() const
{
    return m_allocator_ring.device();
}

CommandList::CommandList(Device& device, 
    CommandType command_workload_type, uint32_t node_mask, 
    FenceSharing command_list_sync_mode/* = FenceSharing::none*/, 
    PipelineState const* initial_pipeline_state/* = nullptr */):
    m_allocator_ring{ CommandAllocatorRingAttorney<CommandList>::makeCommandAllocatorRing(device, command_workload_type) },
    m_node_mask{ node_mask },
    m_signal{ device, command_list_sync_mode }
{
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        device.native()->CreateCommandList(node_mask, static_cast<D3D12_COMMAND_LIST_TYPE>(command_workload_type),
            CommandAllocatorRingAttorney<CommandList>::getCurrentAllocatorInAllocatorRing(m_allocator_ring).Get(),
            initial_pipeline_state ? initial_pipeline_state->native().Get() : NULL, 
            IID_PPV_ARGS(&m_command_list)),
        S_OK);
}

void CommandList::defineSignalingCommandList(CommandList const& signaling_command_list)
{
    CommandAllocatorRingAttorney<CommandList>::attachSignalToCommandAllocatorRing(m_allocator_ring, signaling_command_list.m_signal);
}

Signal const* CommandList::getJobCompletionSignalPtr() const
{
    return CommandAllocatorRingAttorney<CommandList>::getCurrentJobCompletionSignalPtrFromCommandAllocatorRing(m_allocator_ring);
}

TextureCopyLocation::TextureCopyLocation(Resource const& copy_location_resource, uint32_t subresource_index):
    m_copy_location_resource_ref{ copy_location_resource }
{
    m_native_copy_location_desc = static_cast<UINT>(subresource_index);
}

TextureCopyLocation::TextureCopyLocation(Resource const& copy_location_resource, uint64_t resource_offset,
    DXGI_FORMAT resource_format, 
    uint32_t resource_width, uint32_t resource_height, uint32_t resource_depth, uint32_t resource_row_pitch):
    m_copy_location_resource_ref{ copy_location_resource }
{
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresource_footprint;
    subresource_footprint.Offset = static_cast<UINT64>(resource_offset);
    subresource_footprint.Footprint.Format = resource_format,
    subresource_footprint.Footprint.Width = static_cast<UINT>(resource_width);
    subresource_footprint.Footprint.Height = static_cast<UINT>(resource_height);
    subresource_footprint.Footprint.Depth = static_cast<UINT>(resource_depth);
    subresource_footprint.Footprint.RowPitch = static_cast<UINT>(resource_row_pitch);

    m_native_copy_location_desc = subresource_footprint;
}
