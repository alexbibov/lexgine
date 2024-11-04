#ifndef LEXGINE_SCENEGRAPH_MESH_H
#define LEXGINE_SCENEGRAPH_MESH_H

#include <vector>

#include "material.h"
#include "engine/core/vertex_attributes.h"

#include "submesh.h"

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


private:
    
};

}


#endif