#include "engine/core/dx/d3d12/resource.h"

#include "submesh.h"
#include "material.h"

namespace lexgine::scenegraph
{

namespace
{

size_t calculateVertexStrideForTightPacking(core::VertexAttributeSpecificationList const& vertex_attributes)
{
    size_t rv{};
    for (auto const& e : vertex_attributes) {
        rv += e->capacity();
    }
    return rv;
}

}  // namespace

Submesh::Submesh(SceneMeshMemory const& scene_mesh_memory)
    : m_scene_mesh_memory{ scene_mesh_memory }
    , m_instance_count{ 1 }
{

}

void Submesh::setVertexBuffer(
    size_t input_slot,
    SceneMemoryBufferHandle const& buffer_handle,
    core::VertexAttributeSpecificationList const& vertex_attributes,
    size_t vertex_count,
    size_t vertex_stride
)
{
    size_t stride = vertex_stride > 0 ? vertex_stride : calculateVertexStrideForTightPacking(vertex_attributes);
    m_vb_binding.setVertexBufferView(
        static_cast<uint8_t>(input_slot),
        m_scene_mesh_memory.getGPUResource(),
        buffer_handle.offset,
        static_cast<uint32_t>(stride),
        static_cast<uint32_t>(vertex_count)
    );
}

void Submesh::setIndexBuffer(SceneMemoryBufferHandle const& buffer_handle, core::dx::d3d12::IndexDataType index_data_type)
{
    uint32_t const index_size_in_bytes = index_data_type == core::dx::d3d12::IndexDataType::_32_bit ? 4 : 2;
    assert(buffer_handle.size % index_size_in_bytes == 0);
    uint32_t index_count = buffer_handle.size / index_size_in_bytes;
    m_ib_binding = core::dx::d3d12::IndexBufferBinding{
        m_scene_mesh_memory.getGPUResource(),
        static_cast<uint64_t>(buffer_handle.offset),
        index_data_type,
        index_count
    };
}

void Submesh::setBaseMaterial(Material* p_material)
{
    m_base_material_ptr = p_material;
    m_object_parameters_data_mapper = std::make_unique<core::dx::d3d12::ConstantBufferDataMapper>(
        p_material->getStaticState().getObjectParametersUniformBufferReflection()
    );
}

}
