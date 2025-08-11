#include "engine/core/dx/d3d12/command_list.h"
#include "engine/core/dx/d3d12/vertex_buffer_binding.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"

#include "vertex_buffer_view.h"

namespace lexgine::scenegraph
{

namespace
{

size_t calculateVertexStrideForTightPacking(core::VertexAttributeSpecificationList const& vertex_attributes)
{
    size_t rv{};
    for (auto const& e : vertex_attributes) {
        rv += e->size();
    }
    return rv;
}

}


VertexBufferView::VertexBufferView(SceneMeshMemory const& scene_mesh_memory)
    : m_scene_mesh_memory{ scene_mesh_memory }
{

}

void VertexBufferView::setVertexBuffer(
    size_t input_slot,
    SceneMemoryBufferHandle const& buffer_handle,
    core::VertexAttributeSpecificationList const& vertexAttributes,
    size_t vertex_count,
    size_t vertex_stride
)
{
    std::unique_ptr<VertexBufferFormatDesc> vb_desc = std::make_unique<VertexBufferFormatDesc>();
    vb_desc->buffer_handle = buffer_handle;
    vb_desc->vertex_count = vertex_count;
    vb_desc->vertex_attributes = vertexAttributes;
    vb_desc->stride = vertex_stride > 0 ? vertex_stride : calculateVertexStrideForTightPacking(vertexAttributes);
    m_vertex_buffers[input_slot] = std::move(vb_desc);
}

void VertexBufferView::bindBuffers(core::dx::d3d12::CommandList& recording_command_list)
{
    core::dx::d3d12::VertexBufferBinding vb_binding{};
    for (size_t i = 0; i < m_vertex_buffers.size(); ++i)
    {
        auto const& vertex_buffer = m_vertex_buffers[i];
        if (vertex_buffer)
        {
            vb_binding.setVertexBufferView(
                static_cast<uint8_t>(i), 
                m_scene_mesh_memory.getGPUResource(), 
                vertex_buffer->buffer_handle.offset, 
                vertex_buffer->stride,
                vertex_buffer->vertex_count
            );
        }
    }
    recording_command_list.inputAssemblySetVertexBuffers(vb_binding);
}



}