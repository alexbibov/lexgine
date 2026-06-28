#include "mesh.h"


namespace lexgine::scenegraph
{

size_t Mesh::addSubmesh(Submesh&& submesh)
{
    size_t rv = m_submeshes.size();
    //m_submeshes.emplace_back(std::move(submesh));
    return rv;
}

void Mesh::forEachSubmesh(std::function<void(Submesh&)> const& op)
{
    for (Submesh& s : m_submeshes)
    {
        op(s);
    }
}

void Mesh::forEachSubmesh(std::function<void(Submesh const&)> const& op) const
{
    for (Submesh const& s : m_submeshes)
    {
        op(s);
    }
}

}