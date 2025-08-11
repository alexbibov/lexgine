#include "submesh.h"

namespace lexgine::scenegraph
{

Submesh::Submesh(SceneMeshMemory const& scene_mesh_memory)
    : m_vb_view{ new VertexBufferView{scene_mesh_memory} }
    , m_instance_count{ 1 }
{

}

void Submesh::setIndexBuffer(SceneMemoryBufferHandle const& buffer_handle, IndexType index_data_type)
{
    m_ib_view = buffer_handle;
    m_ib_data_type = index_data_type;
}

}