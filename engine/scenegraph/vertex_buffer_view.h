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
public:
    VertexBufferView(core::Globals const& globals, SceneMeshMemory const& scene_mesh_memory);
    void setVertexBuffer(
        size_t input_slot,
        SceneMemoryBufferHandle const& buffer_handle,
        core::VertexAttributeSpecificationList const& vertexAttributes, 
        size_t vertex_count
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

    struct VertexAttributeDesc {
        std::string name;
        core::misc::DataFormat format;
        bool is_normalized;
        VertexAttributeElementCount element_count;

        size_t size() const;
    };

    struct VertexBufferFormatDesc {
        SceneMemoryBufferHandle buffer_handle;
        std::array<core::misc::Optional<VertexAttributeDesc>, 16> vertex_attributes;
        size_t vertex_count;

        size_t vertexStride() const;
    };

private:
    core::dx::d3d12::DxResourceFactory const& m_dx_resource_factory;
    SceneMeshMemory const& m_scene_mesh_memory;
    std::array<core::misc::Optional<VertexBufferFormatDesc>, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> m_vertex_buffers;
};

}

#endif