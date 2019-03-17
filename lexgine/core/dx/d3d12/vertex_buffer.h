#ifndef LEXGINE_CORE_DX_D3D12_VERTEX_BUFFER_H
#define LEXGINE_CORE_DX_D3D12_VERTEX_BUFFER_H

#include <memory>
#include "lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/vertex_attributes.h"
#include "vertex_buffer_binding.h"

namespace lexgine::core::dx::d3d12 {

class VertexBuffer
{
public:
    VertexBuffer(Device const& device,
        uint32_t node_mask = 0x1, uint32_t node_exposure_mask = 0x1,
        bool allow_cross_adapter = false);

    void setSegment(VertexAttributeSpecificationList const& va_spec_list,
        size_t segment_vertex_capacity, uint8_t target_slot);    //! defines a segment of the vertex buffer

    void build();    //! builds the vertex buffer with given segments

private:
    struct vb_segment
    {
        VertexAttributeSpecificationList va_spec_list;
        size_t vertex_capacity;
        uint8_t target_slot;
    };

private:
    Device const& m_device;
    uint32_t m_node_mask;
    uint32_t m_node_exposure_mask;
    bool m_allow_cross_adapter;

    std::list<vb_segment> m_vb_specification;

    std::unique_ptr<CommittedResource> m_vertex_buffer;
    VertexBufferBinding m_vertex_buffer_binding;
};

}

#endif
