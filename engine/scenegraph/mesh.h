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
    Mesh(std::string const& name)
        : m_name{ name }
    {

    }

    Mesh(Mesh const&) = delete;
    Mesh(Mesh&&) noexcept = default;

    ~Mesh() = default;

    Mesh& operator=(Mesh const&) = delete;
    Mesh& operator=(Mesh&&) noexcept = default;

    size_t addSubmesh(Submesh&& submesh);
    void clearSubmeshes() { m_submeshes.clear(); }
    size_t getSubmeshCount() const { return m_submeshes.size(); }

    Submesh const& getSubmesh(size_t index) const { return m_submeshes[index]; }
    Submesh& getSubmesh(size_t index) { return m_submeshes[index]; }

    void forEachSubmesh(std::function<void(Submesh&)> const& op);
    void forEachSubmesh(std::function<void(Submesh const&)> const& op) const;

    void applyMorphWeights(std::vector<double> const& morph_target_weights) { m_morph_weights = morph_target_weights; }

private:
    std::vector<Submesh> m_submeshes;
    std::string m_name;
    std::vector<double> m_morph_weights;
};

}


#endif