#ifndef LEXGINE_CORE_DX_D3D12_VERTEX_BUFFER_BINDING_H
#define LEXGINE_CORE_DX_D3D12_VERTEX_BUFFER_BINDING_H

#include <array>

#include "lexgine_core_dx_d3d12_fwd.h"
#include "command_list.h"

namespace lexgine::core::dx::d3d12 {

class VertexBufferBinding final
{
public:
    VertexBufferBinding() = default;

    void setVertexBufferView(uint8_t input_assembler_slot, Resource const& source_vertex_data_resource, 
        uint64_t vertex_data_offset, uint32_t vertex_entry_stride, uint32_t vertices_count);

    uint16_t slotUsageMask() const;

    D3D12_VERTEX_BUFFER_VIEW const& vertexBufferViewAtSlot(uint8_t slot) const;
    
private:
    std::array<D3D12_VERTEX_BUFFER_VIEW, CommandList::c_input_assembler_count> m_vb_views;
    uint16_t m_defined_slots_mask;
};

enum struct IndexDataType { _16_bit, _32_bit };

class IndexBufferBinding final
{
public:
    IndexBufferBinding(Resource const& source_index_data_resource, uint64_t index_data_offset,
        IndexDataType index_format, uint32_t index_count);

    void update(uint64_t new_index_data_offset, uint32_t new_index_count);

    D3D12_INDEX_BUFFER_VIEW const& indexBufferView() const;

private:
    uint64_t m_index_data_offset;
    D3D12_INDEX_BUFFER_VIEW m_native_ib_view;
};

}

#endif
