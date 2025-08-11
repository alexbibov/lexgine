#ifndef LEXGINE_SCENEGRAPH_VERTEX_BUFFER_VIEW_H
#define LEXGINE_SCENEGRAPH_VERTEX_BUFFER_VIEW_H

#include <array>
#include <string>

#include "engine/core/vertex_attributes.h"
#include "engine/core/globals.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/misc/optional.h"

#include "scene_mesh_memory.h"

namespace lexgine::scenegraph
{

class VertexBufferView
{
    friend class Submesh;

public:
    void setVertexBuffer(
        size_t input_slot,
        SceneMemoryBufferHandle const& buffer_handle,
        core::VertexAttributeSpecificationList const& vertexAttributes, 
        size_t vertex_count, 
        size_t vertex_stride
    );

    void bindBuffers(core::dx::d3d12::CommandList& recording_command_list);

    SceneMeshMemory const& sceneMeshMemory() const { return m_scene_mesh_memory; }


private:
    enum class VertexAttributeElementCount {
        _1 = 1,
        _2,
        _3,
        _4
    };

    struct VertexBufferFormatDesc {
        SceneMemoryBufferHandle buffer_handle;
        core::VertexAttributeSpecificationList vertex_attributes;
        size_t vertex_count;
        size_t stride;
    };

private:
    VertexBufferView(SceneMeshMemory const& scene_mesh_memory);

private:
    SceneMeshMemory const& m_scene_mesh_memory;
    std::array<std::unique_ptr<VertexBufferFormatDesc>, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> m_vertex_buffers;
};

}

#endif