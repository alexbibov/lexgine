#include "mesh.h"
#include "submesh.h"

namespace lexgine::scenegraph
{

uint32_t Mesh::addSubmesh()
{
    size_t rv = m_submeshes.size();
    m_submeshes.emplace_back(m_owner);
    return static_cast<uint32_t>(rv);
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