#ifndef LEXGINE_SCENEGRAPH_MESH_H
#define LEXGINE_SCENEGRAPH_MESH_H

#include <vector>
#include <functional>

#include "material.h"
#include "engine/core/vertex_attributes.h"
#include "engine/scenegraph/scene_mesh_memory.h"
#include "engine/scenegraph/submesh.h"

namespace lexgine::scenegraph
{
class Mesh final : public core::NamedEntity<Mesh>
{   
public:
    Mesh(Scene const& owner, std::string const& name)
        : m_owner{ owner }
    {
        setStringName(name);
    }

    Mesh(Mesh const&) = delete;
    Mesh(Mesh&&) noexcept = default;

    ~Mesh() = default;

    Mesh& operator=(Mesh const&) = delete;
    Mesh& operator=(Mesh&&) noexcept = default;

    Scene const& getScene() const { return m_owner; }

    uint32_t addSubmesh();
    void clearSubmeshes() { m_submeshes.clear(); }
    uint32_t getSubmeshCount() const { return static_cast<uint32_t>(m_submeshes.size()); }

    Submesh const& getSubmesh(uint32_t submesh_id) const { return m_submeshes[submesh_id]; }
    Submesh& getSubmesh(uint32_t submesh_id) { return m_submeshes[submesh_id]; }

    void forEachSubmesh(std::function<void(Submesh&)> const& op);
    void forEachSubmesh(std::function<void(Submesh const&)> const& op) const;

    void applyMorphWeights(std::vector<double> const& morph_target_weights) { m_morph_weights = morph_target_weights; }

private:
    Scene const& m_owner;
    std::vector<Submesh> m_submeshes;
    std::vector<double> m_morph_weights;
};

}


#endif