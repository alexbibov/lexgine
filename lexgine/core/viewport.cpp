#include "viewport.h"

using namespace lexgine::core;

Viewport::Viewport(math::Vector2f const& top_left_corner_coordinates, 
    math::Vector2f const& dimensions, math::Vector2f const& depth_range) :
    m_top_left_corner{ top_left_corner_coordinates },
    m_dimensions{ dimensions },
    m_depth_range{ depth_range }
{
}

math::Vector2f Viewport::topLeftCorner() const
{
    return m_top_left_corner;
}

float Viewport::width() const
{
    return m_dimensions.x;
}

float Viewport::height() const
{
    return m_dimensions.y;
}

math::Vector2f Viewport::depthRange() const
{
    return m_depth_range;
}
