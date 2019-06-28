#ifndef LEXGINE_CORE_VIEWPORT_H
#define LEXGINE_CORE_VIEWPORT_H

#include "lexgine/core/math/vector_types.h"

namespace lexgine::core {

class Viewport final
{
public:
    Viewport(math::Vector2f const& top_left_corner_coordinates,
        math::Vector2f const& dimensions, math::Vector2f const& depth_range);

    math::Vector2f topLeftCorner() const;
    float width() const;
    float height() const;
    math::Vector2f depthRange() const;

    bool operator == (Viewport const& other) const;

private:
    math::Vector2f m_top_left_corner;
    math::Vector2f m_dimensions;
    math::Vector2f m_depth_range;
};

}

#endif
