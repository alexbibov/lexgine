#include <algorithm>
#include "utility.h"
#include "engine/core/exception.h"
#include "engine/core/class_names.h"
#include "engine/core/math/vector_types.h"
#include "engine/core/math/matrix_types.h"

using namespace lexgine::core;
using namespace lexgine::core::math;
using namespace lexgine::core::misc;

namespace lexgine::core::math {

extern double const pi = 3.1415926535897932384626433832795;

Matrix4f createProjectionMatrix(
	float aspect_ratio,
	float vertical_fov/* = 75.0f*/,
	float near_plane_distance/* = 0.1f*/,
	float far_plane_distance/* = 10000.f*/,
	bool invert_depth/* = true*/
)
{
    float f = 1.0f / std::tan(vertical_fov * 0.5f * pi / 180.f);
    Vector4f row0{ f / aspect_ratio, 0.f, 0.f, 0.f };
    Vector4f row1{ 0.f, f, 0.f, 0.f };
    float Q = 1.0f / (far_plane_distance - near_plane_distance);
    float nf = near_plane_distance * far_plane_distance;
    Vector4f row2 = invert_depth 
        ? Vector4f{ 0.f, 0.f, near_plane_distance * Q, nf * Q }
        : Vector4f{ 0.f, 0.f, -far_plane_distance * Q, -nf * Q };
    Vector4f row3{ 0.f, 0.f, -1.f, 0.f };
    return Matrix4f{ row0, row1, row2, row3 }.transpose();
}

Matrix4f createOrthogonalProjectionMatrix(
    float position_x, 
    float position_y,
    float width,
    float height, 
    float near_cutoff_distance,
    float far_cutoff_distance
)
{
    float l{ position_x }, r{ position_x + width };
    float b{ position_y + height }, t{ position_y };
    float n{ near_cutoff_distance }, f{ far_cutoff_distance };
    Vector4f row0{ 2.f / (r - l), 0.f, 0.f, (l + r) / (l - r) };
    Vector4f row1{ 0.f, 2.f / (t - b), 0.f, (b + t) / (b - t) };
    Vector4f row3{ 0.f, 0.f, 0.f, 1.f };
    float Q{ 1.f / (f - n) };
    Vector4f row2{ 0.f, 0.f, Q, f * Q };
    return Matrix4f{ row0, row1, row2, row3 }.transpose();
}

}
