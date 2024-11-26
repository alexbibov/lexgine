#include "engine/core/dx/d3d12/command_list.h"
#include "engine/core/dx/d3d12/vertex_buffer_binding.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"

#include "vertex_buffer_view.h"

namespace lexgine::scenegraph
{

size_t VertexBufferView::VertexAttributeDesc::size() const
{
    unsigned element_size{};
    switch (format)
    {
    case core::misc::DataFormat::float64:
    case core::misc::DataFormat::int64:
    case core::misc::DataFormat::uint64:
        element_size = 8;
        break;
    case core::misc::DataFormat::float32:
    case core::misc::DataFormat::int32:
    case core::misc::DataFormat::uint32:
        element_size = 4;
        break;
    case core::misc::DataFormat::float16:
    case core::misc::DataFormat::int16:
    case core::misc::DataFormat::uint16:
        element_size = 2;
        break;
    case core::misc::DataFormat::int8:
    case core::misc::DataFormat::uint8:
        element_size = 1;
        break;
    default:
        LEXGINE_ASSUME;
    }

    return element_size * static_cast<unsigned>(element_count);
}


size_t VertexBufferView::VertexBufferFormatDesc::vertexStride() const
{
    size_t rv {};
    for (auto const& e : vertex_attributes) {
        if (e.isValid()) {
            rv += e->size();
        }
    }
    return rv;
}


VertexBufferView::VertexBufferView(core::Globals const& globals, SceneMeshMemory const& scene_mesh_memory)
    : m_dx_resource_factory { *globals.get<core::dx::d3d12::DxResourceFactory>() }
    , m_scene_mesh_memory{ scene_mesh_memory }
{

}

void VertexBufferView::setVertexBuffer(
    size_t input_slot,
    SceneMemoryBufferHandle const& buffer_handle,
    core::VertexAttributeSpecificationList const& vertexAttributes,
    size_t vertex_count
)
{
    VertexBufferFormatDesc vb_desc{ .buffer_handle = buffer_handle };
    vb_desc.vertex_count = vertex_count;
    for (size_t i = 0; i < vertexAttributes.size(); ++i) 
    {
        auto const& va_spec = vertexAttributes[i];
        VertexAttributeDesc va_desc{ .name = va_spec->name() };
        va_desc.element_count = static_cast<VertexAttributeElementCount>(va_spec->size());

        auto format_description = m_dx_resource_factory.dxgiFormatFetcher().fetch(va_spec->format<core::EngineApi::Direct3D12>());
        va_desc.is_normalized = format_description.is_normalized;

        switch (format_description.element_size)
        {
        case 1:
            va_desc.format = format_description.is_signed ? core::misc::DataFormat::int8 : core::misc::DataFormat::uint8;
            break;

        case 2:
            if (format_description.is_fp)
            {
                va_desc.format = core::misc::DataFormat::float16;
            }
            else
            {
                va_desc.format = format_description.is_signed ? core::misc::DataFormat::int16 : core::misc::DataFormat::uint16;
            }
            break;

        case 4:
            if (format_description.is_fp) {
                va_desc.format = core::misc::DataFormat::float32;
            } else {
                va_desc.format = format_description.is_signed ? core::misc::DataFormat::int32 : core::misc::DataFormat::uint32;
            }
            break;

        case 8:
            assert(false); // double precision floating point is not supported

        default:
            LEXGINE_ASSUME;
        }

        vb_desc.vertex_attributes[va_spec->input_slot()] = va_desc;
    }
    m_vertex_buffers[input_slot] = vb_desc;
}

void VertexBufferView::bindBuffers(core::dx::d3d12::CommandList& recording_command_list)
{
    core::dx::d3d12::VertexBufferBinding vb_binding{};
    for (size_t i = 0; i < m_vertex_buffers.size(); ++i)
    {
        auto const& vertex_buffer = m_vertex_buffers[i];
        if (vertex_buffer.isValid())
        {
            vb_binding.setVertexBufferView(
                static_cast<uint8_t>(i), 
                m_scene_mesh_memory.getGPUResource(), 
                vertex_buffer->buffer_handle.offset, 
                vertex_buffer->vertexStride(),
                vertex_buffer->vertex_count
            );
        }
    }
    recording_command_list.inputAssemblySetVertexBuffers(vb_binding);
}



}