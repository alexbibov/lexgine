#include <numeric>
#include "vertex_buffer.h"
#include "resource.h"

using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core;


VertexBuffer::VertexBuffer(Device const& device,
    uint32_t node_mask = 0x1, uint32_t node_exposure_mask = 0x1,
    bool allow_cross_adapter = false)
    : m_device{ device }
    , m_node_mask{ node_mask }
    , m_node_exposure_mask{ node_exposure_mask }
    , m_allow_cross_adapter{ allow_cross_adapter }
{
    
}

void VertexBuffer::setSegment(VertexAttributeSpecificationList const& va_spec_list, 
    size_t segment_vertex_capacity, uint8_t target_slot)
{
}


