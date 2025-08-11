#ifndef LEXGINE_SCENEGRAPH_MESH_H
#define LEXGINE_SCENEGRAPH_MESH_H

#include <vector>

#include "material.h"
#include "engine/core/vertex_attributes.h"
#include "engine/scenegraph/scene_mesh_memory.h"
#include "engine/scenegraph/submesh.h"

namespace lexgine::scenegraph
{

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


class Mesh final
{   
public:
    Mesh(std::string const& name)
        : m_name{ name }
    {

    }

    void addSubmesh(Submesh&& submesh) { m_submeshes.emplace_back(std::move(submesh)); }
    void clearSubmeshes() { m_submeshes.clear(); }
    size_t getSubmeshCount() const { return m_submeshes.size(); }

    Submesh const& getSubmesh(size_t index) const { return m_submeshes[index]; }
    Submesh& getSubmesh(size_t index) { return m_submeshes[index]; }

    void applyMorphWeights(std::vector<double> const& morph_target_weights) { m_morph_weights = morph_target_weights; }

private:
    std::vector<Submesh> m_submeshes;
    std::string m_name;
    std::vector<double> m_morph_weights;
    
};

}


#endif