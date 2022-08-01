#ifndef LEXGINE_CORE_MATH_UTILITY_H
#define LEXGINE_CORE_MATH_UTILITY_H

#include "matrix_types.h"
#include "engine/core/engine_api.h"

namespace lexgine::core::math {

extern double const pi;

Matrix4f createProjectionMatrix(EngineApi target_api, float width, float height, 
    float horizontal_fov = 120.f, float cutoff_distance = 10000.f, bool invert_depth = true);

Matrix4f createOrthogonalProjectionMatrix(EngineApi target_api, float position_x, float position_y,
    float widht, float height, float near_cutoff_distance = 1e-3f, float far_cutoff_distance = 1e4f);

}

#endif
