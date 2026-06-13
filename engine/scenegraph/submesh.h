#ifndef LEXGINE_SCENEGRAPH_SUBMESH_H
#define LEXGINE_SCENEGRAPH_SUBMESH_H

#include "engine/core/entity.h"
#include "engine/core/vertex_attributes.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/vertex_buffer_binding.h"
#include "engine/scenegraph/scene_mesh_memory.h"

namespace lexgine::scenegraph
{

class Material;

enum class SubmeshTopology
{
    points,
    line,
    line_loop,
    line_strip,
    triangles,
    triangle_strip,
    triangle_fan
};

class Submesh final : public core::NamedEntity<Submesh>
{
public:
    Submesh(SceneMeshMemory const& scene_mesh_memory);

    void setVertexBuffer(
        size_t input_slot,
        SceneMemoryBufferHandle const& buffer_handle,
        core::VertexAttributeSpecificationList const& vertex_attributes,
        size_t vertex_count,
        size_t vertex_stride
    );

    core::dx::d3d12::VertexBufferBinding const& getVertexBufferBinding() const { return m_vb_binding; }
    core::dx::d3d12::IndexBufferBinding const& getIndexBufferBinding() const { return m_ib_binding; }

    size_t getInstanceCount() const { return m_instance_count; }
    void setIndexBuffer(SceneMemoryBufferHandle const& buffer_handle, core::dx::d3d12::IndexDataType index_data_type);
    void setBaseMaterial(Material* p_material) { m_baseMaterialPtr = p_material; }
    Material* getBaseMaterial() const { return m_baseMaterialPtr; }

private:
    SceneMeshMemory const& m_scene_mesh_memory;
    core::dx::d3d12::VertexBufferBinding m_vb_binding;
    core::dx::d3d12::IndexBufferBinding m_ib_binding;
    size_t m_instance_count;
    Material* m_baseMaterialPtr{ nullptr };
};

}

#endif
