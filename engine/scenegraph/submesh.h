#ifndef LEXGINE_SCENEGRAPH_SUBMESH_H
#define LEXGINE_SCENEGRAPH_SUBMESH_H

#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"

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
    Submesh(VertexBufferView const& vertex_buffer_view, size_t instance_count);
    void setIndexBuffer(MeshBufferHandle const& buffer_handle, IndexType index_data_type);

    void draw(core::dx::d3d12::CommandList& recording_command_list) const;

private:
    VertexBufferView m_vb_view;
    size_t m_instance_count;
    IndexType m_ib_data_type;
    MeshBufferHandle m_ib_view;
};

}

#endif
