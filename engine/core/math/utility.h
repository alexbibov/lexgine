#ifndef LEXGINE_CORE_MATH_UTILITY_H
#define LEXGINE_CORE_MATH_UTILITY_H

#include "matrix_types.h"
#include "engine/core/engine_api.h"

namespace lexgine::core::math {

extern double const pi;

Matrix4f createProjectionMatrix(
    float aspect_ratio,
    float vertical_fov = 75.0f,
    float near_plane_distance = 0.1f,
    float far_plane_distance = 10000.f, 
    bool invert_depth = true
);

Matrix4f createOrthogonalProjectionMatrix(
    float position_x, 
    float position_y,
    float widht, 
    float height, 
    float near_cutoff_distance = 1e-3f, 
    float far_cutoff_distance = 1e4f
);

}

#endif
