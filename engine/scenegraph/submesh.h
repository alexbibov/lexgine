#ifndef LEXGINE_SCENEGRAPH_SUBMESH_H
#define LEXGINE_SCENEGRAPH_SUBMESH_H

#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/tasks/pso_compilation_task.h"
#include "vertex_buffer_view.h"

namespace lexgine::scenegraph
{

enum class IndexType
{
    _default,
    _short
};


class Submesh final
{
public:
    Submesh(SceneMeshMemory const& scene_mesh_memory);

    VertexBufferView* getVertexBufferView() { return m_vb_view.get(); }
    size_t getInstanceCount() const { return m_instance_count; }

    void setIndexBuffer(SceneMemoryBufferHandle const& buffer_handle, IndexType index_data_type);
    


    void draw(core::dx::d3d12::CommandList& recording_command_list) const;

private:
    std::unique_ptr<VertexBufferView> m_vb_view;
    size_t m_instance_count;
    IndexType m_ib_data_type;
    SceneMemoryBufferHandle m_ib_view;

    core::dx::d3d12::GraphicsPSODescriptor m_pso_descriptor;
    core::dx::d3d12::tasks::GraphicsPSOCompilationTask* m_pso_compilation_task { nullptr };
};

}

#endif
