#include <algorithm>
#include "utility.h"
#include "engine/core/exception.h"
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
    float far_cutoff_distance,
    bool invert_depth
)
{
    float l{ position_x }, r{ position_x + width };
    float b{ position_y + height }, t{ position_y };
    float n{ near_cutoff_distance }, f{ far_cutoff_distance };
    Vector4f row0{ 2.f / (r - l), 0.f, 0.f, (l + r) / (l - r) };
    Vector4f row1{ 0.f, 2.f / (t - b), 0.f, (b + t) / (b - t) };
    Vector4f row3{ 0.f, 0.f, 0.f, 1.f };
    float Q{ 1.f / (f - n) };
    Vector4f row2 = invert_depth
        ? Vector4f{ 0.f, 0.f, Q, f * Q }
        : Vector4f{ 0.f, 0.f, -Q, -n * Q };
    return Matrix4f{ row0, row1, row2, row3 }.transpose();
}

float length(Vector2f const& v)
{
    return glm::length(glm::vec2{ v });
}

float length(Vector3f const& v)
{
    return glm::length(glm::vec3{ v });
}

float length(Vector4f const& v)
{
    return glm::length(glm::vec4{ v });
}

double length(Vector2d const& v)
{
    return glm::length(glm::dvec2{ v });
}

double length(Vector3d const& v)
{
    return glm::length(glm::dvec3{ v });
}

double length(Vector4d const& v)
{
    return glm::length(glm::dvec4{ v });
}

void decomposeTransform(
    Matrix4f const& transform,
    Vector3f& translation,
    Quaternionf& rotation,
    Vector3f& scale
)
{
    translation = Vector3f{ transform[3] };

    scale = Vector3f{
        length(Vector3f{ transform[0] }),
        length(Vector3f{ transform[1] }),
        length(Vector3f{ transform[2] })
    };

    // A negative determinant indicates a flipped axis; fold the flip into the X scale component
    if (transform.determinant() < 0.f)
    {
        scale.x = -scale.x;
    }

    float const safe_scale_x = scale.x != 0.f ? scale.x : 1.f;
    float const safe_scale_y = scale.y != 0.f ? scale.y : 1.f;
    float const safe_scale_z = scale.z != 0.f ? scale.z : 1.f;
    Matrix3f const rotation_matrix{
        Vector3f{ transform[0] } / safe_scale_x,
        Vector3f{ transform[1] } / safe_scale_y,
        Vector3f{ transform[2] } / safe_scale_z
    };

    rotation = glm::quat_cast(rotation_matrix);
}

}
