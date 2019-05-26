#include "vertex_buffer_binding.h"
#include "resource.h"

using namespace lexgine::core::dx::d3d12;

void VertexBufferBinding::setVertexBufferView(uint8_t input_assembler_slot, Resource const& source_vertex_data_resource, 
    uint64_t vertex_data_offset, uint32_t vertex_entry_stride, uint32_t vertices_count)
{
    m_defined_slots_mask |= (1 << input_assembler_slot);

    D3D12_VERTEX_BUFFER_VIEW vb_view;
    vb_view.BufferLocation = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(source_vertex_data_resource.getGPUVirtualAddress() + vertex_data_offset);
    vb_view.SizeInBytes = vertex_entry_stride * vertices_count;
    vb_view.StrideInBytes = vertex_entry_stride;

    m_vb_views[input_assembler_slot] = vb_view;
}

uint16_t VertexBufferBinding::slotUsageMask() const
{
    return m_defined_slots_mask;
}

D3D12_VERTEX_BUFFER_VIEW const& VertexBufferBinding::vertexBufferViewAtSlot(uint8_t slot) const
{
    return m_vb_views[slot];
}

IndexBufferBinding::IndexBufferBinding(Resource const& source_index_data_resource, uint64_t index_data_offset,
    IndexDataType index_format, uint32_t index_count)
    : m_index_data_offset{ index_data_offset }
{
    m_native_ib_view.BufferLocation = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(source_index_data_resource.getGPUVirtualAddress() + m_index_data_offset);

    switch (index_format)
    {
    case IndexDataType::_16_bit:
        m_native_ib_view.Format = DXGI_FORMAT_R16_UINT;
        m_native_ib_view.SizeInBytes = 2 * index_count;
        break;
    case IndexDataType::_32_bit:
        m_native_ib_view.Format = DXGI_FORMAT_R32_UINT;
        m_native_ib_view.SizeInBytes = 4 * index_count;
        break;
    default:
        __assume(0);
    }
}

void IndexBufferBinding::update(uint64_t new_index_data_offset, uint32_t new_index_count)
{
    switch (m_native_ib_view.Format)
    {
    case DXGI_FORMAT_R16_UINT:
        m_native_ib_view.SizeInBytes = 2 * new_index_count;
        break;

    case DXGI_FORMAT_R32_UINT:
        m_native_ib_view.SizeInBytes = 4 * new_index_count;
        break;

    default:
        __assume(0);
    }

    m_native_ib_view.BufferLocation = m_native_ib_view.BufferLocation - m_index_data_offset + new_index_data_offset;
}

D3D12_INDEX_BUFFER_VIEW const& IndexBufferBinding::indexBufferView() const
{
    return m_native_ib_view;
}
