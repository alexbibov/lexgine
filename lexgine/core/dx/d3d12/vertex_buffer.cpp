#include <numeric>
#include "vertex_buffer.h"

using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::misc;
using namespace lexgine::core;


VertexBuffer::VertexBuffer(Device const& device,
    uint32_t node_mask/* = 0x1*/, uint32_t node_exposure_mask/* = 0x1*/,
    bool allow_cross_adapter/* = false*/)
    : m_device{ device }
    , m_node_mask{ node_mask }
    , m_node_exposure_mask{ node_exposure_mask }
    , m_allow_cross_adapter{ allow_cross_adapter }
{
    
}

void VertexBuffer::setSegment(VertexAttributeSpecificationList const& va_spec_list, 
    uint32_t segment_num_vertices, uint8_t target_slot)
{
    m_vb_specification.push_back(vb_segment{ va_spec_list, segment_num_vertices, target_slot });
}

void VertexBuffer::build()
{
    std::vector<size_t> segment_offsets(m_vb_specification.size() + 1); segment_offsets[0] = 0;
    std::vector<uint32_t> segment_per_vertex_strides(m_vb_specification.size());
    std::vector<uint32_t> segment_vertex_capacities(m_vb_specification.size());
    size_t counter{ 0 };

    for (auto const& segment : m_vb_specification)
    {
        segment_per_vertex_strides[counter] = 
            std::accumulate(segment.va_spec_list.begin(), segment.va_spec_list.end(),
            0U, [](uint32_t size, std::shared_ptr<AbstractVertexAttributeSpecification> const& spec) -> uint32_t
            {
                return size + spec->capacity();
            }
        );

        segment_vertex_capacities[counter] = segment.num_vertices;

        segment_offsets[counter + 1] = segment_offsets[counter] + segment_per_vertex_strides[counter] * segment_vertex_capacities[counter];
        ++counter;
    }

    ResourceFlags vb_resource_flags = ResourceFlags::enum_type::deny_shader_resource;
    HeapCreationFlags vb_heap_flags = HeapCreationFlags::enum_type::allow_only_buffers;
    if (m_allow_cross_adapter)
    {
        vb_resource_flags |= ResourceFlags::enum_type::allow_cross_adapter;
        vb_heap_flags |= HeapCreationFlags::enum_type::shared_cross_adapter;
    }
    ResourceDescriptor vb_descritptor = ResourceDescriptor::CreateBuffer(segment_offsets.back(), vb_resource_flags);

    m_vertex_buffer = std::make_unique<CommittedResource>(m_device, defaultState(),
        Optional<ResourceOptimizedClearValue>{}, vb_descritptor, AbstractHeapType::default,
        vb_heap_flags, m_node_mask, m_node_exposure_mask);

    counter = 0;
    for (auto const& segment : m_vb_specification)
    {
        m_vertex_buffer_binding.setVertexBufferView(segment.target_slot, *m_vertex_buffer,
            segment_offsets[counter], segment_per_vertex_strides[counter], segment_vertex_capacities[counter]);
        ++counter;
    }
}

CommittedResource const& VertexBuffer::resource() const
{
    return *m_vertex_buffer;
}

void VertexBuffer::bind(CommandList& command_list) const
{
    command_list.inputAssemblySetVertexBuffers(m_vertex_buffer_binding);
}



