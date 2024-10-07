#ifndef LEXGINE_SCENEGRAPH_MESH_H

#include <vector>

#include "material.h"

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


class Primitive 
{
public:
    Primitive();
};

class Mesh
{
public:
    Mesh();


private:


};

}


#define LEXGINE_SCENEGRAPH_MESH_H
#endif