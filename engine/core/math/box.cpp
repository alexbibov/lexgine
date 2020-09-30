#include "box.h"

using namespace lexgine::core::math;

Box::Box(float center_x, float center_y, float center_z,
    float width, float height, float depth) :
    m_center{ center_x, center_y, center_z },
    m_extents{ width*.5f, height*.5f, depth*.5f }
{
}

Box::Box(Vector3f const& far_lower_left_vertex_coordinates, 
    Vector3f const& near_top_right_vertex_coordinates) :
    m_center{ (near_top_right_vertex_coordinates + far_lower_left_vertex_coordinates) * .5f },
    m_extents{ (near_top_right_vertex_coordinates - far_lower_left_vertex_coordinates) * .5f }
{
}

Vector3f Box::center() const
{
    return m_center;
}

Vector3f Box::extents() const
{
    return m_extents;
}

Vector3f Box::dimensions() const
{
    return m_extents * 2.f;
}
