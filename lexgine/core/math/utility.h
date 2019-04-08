#ifndef LEXGINE_CORE_MATH_UTILITY_H
#define LEXGINE_CORE_MATH_UTILITY_H

#include "matrix_types.h"
#include "lexgine/core/misc/misc.h"

namespace lexgine::core::math {

extern double const pi;

Matrix4f createProjectionMatrix(misc::EngineAPI target_api, float width, float height, 
    float horizontal_fov = 120.f, float cutoff_distance = 10000.f, bool invert_depth = true);

}

#endif
