#include "mesh.h"


namespace lexgine::scenegraph
{

size_t Mesh::addSubmesh(Submesh&& submesh)
{
    size_t rv = m_submeshes.size();
    m_submeshes.emplace_back(std::move(submesh));
    return rv;
}

}