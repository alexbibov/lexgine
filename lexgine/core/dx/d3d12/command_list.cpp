#include <algorithm>
#include <cassert>

#include "command_list.h"
#include "device.h"
#include "pipeline_state.h"
#include "resource.h"
#include "d3d12_tools.h"
#include "descriptor_table_builders.h"
#include "vertex_buffer_binding.h"


#include "lexgine/core/exception.h"
#include "lexgine/core/viewport.h"
#include "lexgine/core/math/box.h"




using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;


namespace {

RECT convertRectangleToDXGINativeRECT(math::Rectangle const& rectangle)
{
    D3D12_RECT rv{};

    auto ul = rectangle.upperLeft();
    rv.left = static_cast<LONG>(ul.x);
    rv.top = static_cast<LONG>(ul.y);

    auto dims = rectangle.size();
    rv.right = static_cast<LONG>(ul.x + dims.x);
    rv.bottom = static_cast<LONG>(ul.y + dims.y);

    return rv;
}

}


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

    m_initial_pipeline_state = initial_pipeline_state;
}

void CommandList::close() const
{
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_command_list->Close(),
        S_OK);
}

void CommandList::clearState() const
{
    m_command_list->ClearState(m_initial_pipeline_state->native().Get());
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
    uint8_t native_topology = d3d12Convert(primitive_topology);
    

    m_command_list->IASetPrimitiveTopology(static_cast<D3D12_PRIMITIVE_TOPOLOGY>(native_topology));
}

void CommandList::inputAssemblySetVertexBuffers(VertexBufferBinding const& vb_binding)
{
    D3D12_VERTEX_BUFFER_VIEW native_vb_views[c_input_assemblers_count];

    unsigned long index{ 0 };
    size_t offset{ 0 };
    size_t offset_old{ 0 };
    size_t base{ 0 };
    for (unsigned long mask = vb_binding.slotUsageMask();
        _BitScanForward(&index, mask); 
        mask >>= 1, offset += index + 1)
    {
        if (offset - offset_old <= 1)
            native_vb_views[offset] = vb_binding.vertexBufferViewAtSlot(static_cast<uint8_t>(offset));
        else
        {
            m_command_list->IASetVertexBuffers(static_cast<UINT>(base), static_cast<UINT>(offset - base), &native_vb_views[base]);
            base = offset;
        }

        offset_old = offset;
    }
}

void CommandList::inputAssemblySetIndexBuffer(IndexBufferBinding const& ib_binding)
{
    D3D12_INDEX_BUFFER_VIEW const& native_ib_view = ib_binding.indexBufferView();
    m_command_list->IASetIndexBuffer(&native_ib_view);
}

void CommandList::rasterizerStateSetViewports(misc::StaticVector<Viewport, c_maximal_viewport_count> const& viewports) const
{
    misc::StaticVector<D3D12_VIEWPORT, c_maximal_viewport_count> native_viewports(viewports.size());

    std::transform(viewports.begin(), viewports.end(), native_viewports.begin(),
        [](Viewport const& v)
        {
            D3D12_VIEWPORT rv{};

            auto top_left_corner = v.topLeftCorner();
            rv.TopLeftX = static_cast<FLOAT>(top_left_corner.x);
            rv.TopLeftY = static_cast<FLOAT>(top_left_corner.y);

            rv.Width = static_cast<FLOAT>(v.width());
            rv.Height = static_cast<FLOAT>(v.height());

            auto depth_range = v.depthRange();
            rv.MinDepth = static_cast<FLOAT>(depth_range.x);
            rv.MaxDepth = static_cast<FLOAT>(depth_range.y);

            return rv;
        }
    );

    m_command_list->RSSetViewports(static_cast<UINT>(viewports.size()), native_viewports.data());
}

void CommandList::rasterizerStateSetScissorRectangles(misc::StaticVector<math::Rectangle, c_maximal_scissor_rectangles_count> const& rectangles) const
{
    misc::StaticVector<D3D12_RECT, c_maximal_scissor_rectangles_count> native_rects(rectangles.size());
    std::transform(rectangles.begin(), rectangles.end(), native_rects.begin(), convertRectangleToDXGINativeRECT);
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

void CommandList::outputMergerSetRenderTargets(RenderTargetViewDescriptorTable const* rtv_descriptor_table,
    uint64_t active_rtv_descriptors_mask, DepthStencilViewDescriptorTable const* dsv_descriptor_table,
    uint32_t dsv_descriptor_table_offset) const
{
    UINT rtv_count = misc::getSetBitCount(active_rtv_descriptors_mask);

    assert(!rtv_descriptor_table && active_rtv_descriptors_mask == 0
        || rtv_descriptor_table && rtv_descriptor_table->descriptor_count <= c_maximal_rtv_descriptor_table_length
        && rtv_count <= c_maximal_simultaneous_render_targets_count);

    assert(!dsv_descriptor_table || dsv_descriptor_table_offset < dsv_descriptor_table->descriptor_count);

    misc::StaticVector<D3D12_CPU_DESCRIPTOR_HANDLE, c_maximal_simultaneous_render_targets_count> rtv_cpu_handles{};
    D3D12_CPU_DESCRIPTOR_HANDLE* p_rtv_base_cpu_handle{ NULL };
    if (rtv_descriptor_table && active_rtv_descriptors_mask)
    {
        unsigned long idx{ 0 }, i{ 0 };
        for (unsigned long offset = 0;
            _BitScanForward64(&idx, active_rtv_descriptors_mask);
            offset += idx, active_rtv_descriptors_mask >>= idx + 1, ++i)
        {
            rtv_cpu_handles.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ rtv_descriptor_table->cpu_pointer
                + rtv_descriptor_table->descriptor_size*offset });
        }
        p_rtv_base_cpu_handle = rtv_cpu_handles.data();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv_base_cpu_handle{ NULL };
    D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_base_cpu_handle{ NULL };
    if (dsv_descriptor_table)
    {
        dsv_base_cpu_handle.ptr = dsv_descriptor_table->cpu_pointer
            + dsv_descriptor_table->descriptor_size*dsv_descriptor_table_offset;
        p_dsv_base_cpu_handle = &dsv_base_cpu_handle;
    }

    m_command_list->OMSetRenderTargets(rtv_count, p_rtv_base_cpu_handle, FALSE, p_dsv_base_cpu_handle);
}

void CommandList::clearDepthStencilView(DepthStencilViewDescriptorTable const& dsv_descriptor_table,
    uint32_t dsv_descriptor_table_offset, DSVClearFlags clear_flags, 
    float depth_clear_value, uint8_t stencil_clear_value, 
    misc::StaticVector<math::Rectangle, c_maximal_clear_rectangles_count> const& clear_rectangles) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_cpu_handle{ dsv_descriptor_table.cpu_pointer
            + dsv_descriptor_table.descriptor_size*dsv_descriptor_table_offset };
    D3D12_CLEAR_FLAGS flags = static_cast<D3D12_CLEAR_FLAGS>(clear_flags);

    misc::StaticVector<RECT, c_maximal_clear_rectangles_count> native_clear_rectangles(clear_rectangles.size());
    std::transform(clear_rectangles.begin(), clear_rectangles.end(),
        native_clear_rectangles.begin(), convertRectangleToDXGINativeRECT);

    m_command_list->ClearDepthStencilView(dsv_cpu_handle, flags, depth_clear_value, stencil_clear_value,
        static_cast<UINT>(clear_rectangles.size()), clear_rectangles.size() ? native_clear_rectangles.data() : NULL);
}

void CommandList::clearRenderTargetView(RenderTargetViewDescriptorTable const& rtv_descriptor_table, 
    uint32_t rtv_descriptor_table_offset, math::Vector4f const& rgba_clear_value, 
    misc::StaticVector<math::Rectangle, c_maximal_clear_rectangles_count> const& clear_rectangles) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_cpu_descriptor{ rtv_descriptor_table.cpu_pointer
        + rtv_descriptor_table.descriptor_size*rtv_descriptor_table_offset };

    misc::StaticVector<RECT, c_maximal_clear_rectangles_count> native_clear_rectangles(clear_rectangles.size());
    std::transform(clear_rectangles.begin(), clear_rectangles.end(),
        native_clear_rectangles.begin(), convertRectangleToDXGINativeRECT);

    m_command_list->ClearRenderTargetView(rtv_cpu_descriptor, rgba_clear_value.getDataAsArray(),
        static_cast<UINT>(clear_rectangles.size()), clear_rectangles.size() ? native_clear_rectangles.data() : NULL);
}

void CommandList::clearUnorderedAccessView(ShaderResourceDescriptorTable const& uav_descriptor_table, 
    uint32_t uav_descriptor_table_offset, Resource const& resource_to_clear, math::Vector4u const& rgba_clear_value, 
    misc::StaticVector<math::Rectangle, c_maximal_clear_rectangles_count> const& clear_rectangles) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE uav_cpu_descriptor{ uav_descriptor_table.cpu_pointer
        + uav_descriptor_table.descriptor_size*uav_descriptor_table_offset };

    D3D12_GPU_DESCRIPTOR_HANDLE uav_gpu_descriptor{ uav_descriptor_table.gpu_pointer
        + uav_descriptor_table.descriptor_size*uav_descriptor_table_offset };

    misc::StaticVector<RECT, c_maximal_clear_rectangles_count> native_clear_rectangles(clear_rectangles.size());
    std::transform(clear_rectangles.begin(), clear_rectangles.end(),
        native_clear_rectangles.begin(), convertRectangleToDXGINativeRECT);

    m_command_list->ClearUnorderedAccessViewUint(uav_gpu_descriptor, uav_cpu_descriptor,
        resource_to_clear.native().Get(), rgba_clear_value.getDataAsArray(),
        static_cast<UINT>(clear_rectangles.size()), clear_rectangles.size() ? native_clear_rectangles.data() : NULL);
}

void CommandList::clearUnorderedAccessView(ShaderResourceDescriptorTable const& uav_descriptor_table, 
    uint32_t uav_descriptor_table_offset, Resource const& resource_to_clear, math::Vector4f const& rgba_clear_value, 
    misc::StaticVector<math::Rectangle, c_maximal_clear_rectangles_count> const& clear_rectangles) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE uav_cpu_descriptor{ uav_descriptor_table.cpu_pointer
        + uav_descriptor_table.descriptor_size*uav_descriptor_table_offset };

    D3D12_GPU_DESCRIPTOR_HANDLE uav_gpu_descriptor{ uav_descriptor_table.gpu_pointer
        + uav_descriptor_table.descriptor_size*uav_descriptor_table_offset };

    misc::StaticVector<RECT, c_maximal_clear_rectangles_count> native_clear_rectangles(clear_rectangles.size());
    std::transform(clear_rectangles.begin(), clear_rectangles.end(),
        native_clear_rectangles.begin(), convertRectangleToDXGINativeRECT);

    m_command_list->ClearUnorderedAccessViewFloat(uav_gpu_descriptor, uav_cpu_descriptor,
        resource_to_clear.native().Get(), rgba_clear_value.getDataAsArray(),
        static_cast<UINT>(clear_rectangles.size()), clear_rectangles.size() ? native_clear_rectangles.data() : NULL);
}

void CommandList::setPipelineState(PipelineState const& pipeline_state) const
{
    m_command_list->SetPipelineState(pipeline_state.native().Get());
}

void CommandList::setDescriptorHeaps(misc::StaticVector<DescriptorHeap const*, static_cast<size_t>(DescriptorHeapType::count)> const& descriptor_heaps) const
{
    auto cmd_list_type = commandType();

    // formally SetDescriptorHeaps(...) API is supported by command list bundles, but the heaps set from
    // the bundle must correspond to the heaps set by the invoking command list, so this API is 
    // essentially useless for bundles and therefore, we disallow it
    assert((cmd_list_type == CommandType::direct || cmd_list_type == CommandType::compute));

    misc::StaticVector<ID3D12DescriptorHeap*, static_cast<size_t>(DescriptorHeapType::count)> native_descriptor_heaps(descriptor_heaps.size());
    std::transform(descriptor_heaps.begin(), descriptor_heaps.end(), native_descriptor_heaps.begin(),
        [](DescriptorHeap const* dh) { return dh->native().Get(); });

    m_command_list->SetDescriptorHeaps(static_cast<UINT>(descriptor_heaps.size()), native_descriptor_heaps.data());
}


void CommandList::setRootSignature(std::string const& cached_root_signature_friendly_name, 
    BundleInvocationContext bundle_invokation_context) const
{
    auto cmd_list_type = commandType();

    assert((cmd_list_type == CommandType::direct || cmd_list_type == CommandType::compute) 
        && bundle_invokation_context == BundleInvocationContext::none
        || cmd_list_type == CommandType::bundle 
        && (bundle_invokation_context == BundleInvocationContext::direct || bundle_invokation_context == BundleInvocationContext::compute));

    auto rs = device().retrieveRootSignature(cached_root_signature_friendly_name, m_node_mask);

    if (!rs)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Cannot set root signature for command list \""
        + getStringName() + "\": the root signature is identified to have friendly name \"" + cached_root_signature_friendly_name
        + "\", but no root signature with this friendly name and the node mask " + std::to_string(m_node_mask)
        + " exists in the device cache. This usually means that the root signature has not been created for "
        " the node mask required by the command list attempting to set this root signature");
    }
    
    cmd_list_type == CommandType::direct || bundle_invokation_context == BundleInvocationContext::direct
        ? m_command_list->SetGraphicsRootSignature(rs.Get())
        : m_command_list->SetComputeRootSignature(rs.Get());
}

void CommandList::setRootDescriptorTable(uint32_t root_signature_slot, 
    ShaderResourceDescriptorTable const& cbv_srv_uav_table,
    BundleInvocationContext bundle_invokation_context) const
{
    auto cmd_list_type = commandType();

    assert((cmd_list_type == CommandType::direct || cmd_list_type == CommandType::compute)
        && bundle_invokation_context == BundleInvocationContext::none
        || cmd_list_type == CommandType::bundle
        && (bundle_invokation_context == BundleInvocationContext::direct || bundle_invokation_context == BundleInvocationContext::compute));

    cmd_list_type == CommandType::direct || bundle_invokation_context == BundleInvocationContext::direct
        ? m_command_list->SetGraphicsRootDescriptorTable(static_cast<UINT>(root_signature_slot),
            D3D12_GPU_DESCRIPTOR_HANDLE{ cbv_srv_uav_table.gpu_pointer })
        : m_command_list->SetComputeRootDescriptorTable(static_cast<UINT>(root_signature_slot),
            D3D12_GPU_DESCRIPTOR_HANDLE{ cbv_srv_uav_table.gpu_pointer });
}

void CommandList::setRoot32BitConstant(uint32_t root_signature_slot, uint32_t data, 
    uint32_t offset_in_32_bit_values, BundleInvocationContext bundle_invokation_context) const
{
    auto cmd_list_type = commandType();

    assert((cmd_list_type == CommandType::direct || cmd_list_type == CommandType::compute)
        && bundle_invokation_context == BundleInvocationContext::none
        || cmd_list_type == CommandType::bundle
        && (bundle_invokation_context == BundleInvocationContext::direct || bundle_invokation_context == BundleInvocationContext::compute));

    cmd_list_type == CommandType::direct || bundle_invokation_context == BundleInvocationContext::direct
        ? m_command_list->SetGraphicsRoot32BitConstant(static_cast<UINT>(root_signature_slot),
            static_cast<UINT>(data), static_cast<UINT>(offset_in_32_bit_values))
        : m_command_list->SetComputeRoot32BitConstant(static_cast<UINT>(root_signature_slot),
            static_cast<UINT>(data), static_cast<UINT>(offset_in_32_bit_values));
}

void CommandList::setRoot32BitConstants(uint32_t root_signature_slot, std::vector<uint32_t> const& data, 
    uint32_t offset_in_32_bit_values, BundleInvocationContext bundle_invokation_context) const
{
    auto cmd_list_type = commandType();

    assert((cmd_list_type == CommandType::direct || cmd_list_type == CommandType::compute)
        && bundle_invokation_context == BundleInvocationContext::none
        || cmd_list_type == CommandType::bundle
        && (bundle_invokation_context == BundleInvocationContext::direct || bundle_invokation_context == BundleInvocationContext::compute));

    cmd_list_type == CommandType::direct || bundle_invokation_context == BundleInvocationContext::direct
        ? m_command_list->SetGraphicsRoot32BitConstants(static_cast<UINT>(root_signature_slot),
            static_cast<UINT>(data.size()), data.data(), static_cast<UINT>(offset_in_32_bit_values))
        : m_command_list->SetComputeRoot32BitConstants(static_cast<UINT>(root_signature_slot),
            static_cast<UINT>(data.size()), data.data(), static_cast<UINT>(offset_in_32_bit_values));
}

void CommandList::setRootShaderResourceView(uint32_t root_signature_slot, 
    uint64_t gpu_virtual_address,
    BundleInvocationContext bundle_invokation_context) const
{
    auto cmd_list_type = commandType();

    assert((cmd_list_type == CommandType::direct || cmd_list_type == CommandType::compute)
        && bundle_invokation_context == BundleInvocationContext::none
        || cmd_list_type == CommandType::bundle
        && (bundle_invokation_context == BundleInvocationContext::direct || bundle_invokation_context == BundleInvocationContext::compute));

    cmd_list_type == CommandType::direct || bundle_invokation_context == BundleInvocationContext::direct
        ? m_command_list->SetGraphicsRootShaderResourceView(static_cast<UINT>(root_signature_slot),
            static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(gpu_virtual_address))
        : m_command_list->SetComputeRootShaderResourceView(static_cast<UINT>(root_signature_slot),
            static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(gpu_virtual_address));
}

void CommandList::setRootUnorderedAccessView(uint32_t root_signature_slot, 
    uint64_t gpu_virtual_address,
    BundleInvocationContext bundle_invokation_context) const
{
    auto cmd_list_type = commandType();

    assert((cmd_list_type == CommandType::direct || cmd_list_type == CommandType::compute)
        && bundle_invokation_context == BundleInvocationContext::none
        || cmd_list_type == CommandType::bundle
        && (bundle_invokation_context == BundleInvocationContext::direct || bundle_invokation_context == BundleInvocationContext::compute));

    cmd_list_type == CommandType::direct || bundle_invokation_context == BundleInvocationContext::direct
        ? m_command_list->SetGraphicsRootUnorderedAccessView(static_cast<UINT>(root_signature_slot),
            static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(gpu_virtual_address))
        : m_command_list->SetComputeRootUnorderedAccessView(static_cast<UINT>(root_signature_slot),
            static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(gpu_virtual_address));
}

void CommandList::setRootConstantBufferView(uint32_t root_signature_slot, 
    uint64_t gpu_virtual_address,
    BundleInvocationContext bundle_invokation_context) const
{
    auto cmd_list_type = commandType();

    assert((cmd_list_type == CommandType::direct || cmd_list_type == CommandType::compute)
        && bundle_invokation_context == BundleInvocationContext::none
        || cmd_list_type == CommandType::bundle
        && (bundle_invokation_context == BundleInvocationContext::direct || bundle_invokation_context == BundleInvocationContext::compute));

    cmd_list_type == CommandType::direct || bundle_invokation_context == BundleInvocationContext::direct
        ? m_command_list->SetGraphicsRootConstantBufferView(static_cast<UINT>(root_signature_slot),
            static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(gpu_virtual_address))
        : m_command_list->SetComputeRootConstantBufferView(static_cast<UINT>(root_signature_slot),
            static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(gpu_virtual_address));
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
    PipelineState const* initial_pipeline_state/* = nullptr */) :
    m_allocator_ring{ CommandAllocatorRingAttorney<CommandList>::makeCommandAllocatorRing(device, command_workload_type) },
    m_node_mask{ node_mask },
    m_signal{ device, command_list_sync_mode },
    m_initial_pipeline_state{ initial_pipeline_state }
{
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        device.native()->CreateCommandList(node_mask, static_cast<D3D12_COMMAND_LIST_TYPE>(command_workload_type),
            CommandAllocatorRingAttorney<CommandList>::getCurrentAllocatorInAllocatorRing(m_allocator_ring).Get(),
            initial_pipeline_state ? initial_pipeline_state->native().Get() : NULL, 
            IID_PPV_ARGS(&m_command_list)),
        S_OK);

    close();
}

void CommandList::defineSignalingCommandList(CommandList& signaling_command_list)
{
    CommandAllocatorRingAttorney<CommandList>::attachSignalToCommandAllocatorRing(m_allocator_ring, signaling_command_list.m_signal);
}

Signal* CommandList::getJobCompletionSignalPtr()
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
