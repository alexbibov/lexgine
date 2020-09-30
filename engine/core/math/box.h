#ifndef LEXGINE_CORE_MATH_BOX_H
#define LEXGINE_CORE_MATH_BOX_H

#include <cstdint>
#include <array>

#include "vector_types.h"


namespace lexgine::core::math {

class Box
{
public:
    Box(float center_x, float center_y, float center_z,
        float width, float height, float depth);

    Box(Vector3f const& far_lower_left_vertex_coordinates,
        Vector3f const& near_top_right_vertex_coordinates);

    Vector3f center() const;
    Vector3f extents() const;
    Vector3f dimensions() const;

private:
    Vector3f m_center;
    Vector3f m_extents;

};

}


#endif
