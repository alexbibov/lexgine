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



Matrix4f createProjectionMatrix(EngineAPI target_api, float width, float height,
    float horizontal_fov/* = 120.f*/, float cutoff_distance/* = 10000.f*/, bool invert_depth/* = true*/)
{
    float m = (std::max)(width, height);
    width /= m; height /= m;

    double alpha = pi / 360.*horizontal_fov;
    float n = static_cast<float>(width*.5 / std::tan(alpha));
    float f = cutoff_distance;

    Vector4f row0{ 2 * n / width, 0.f, 0.f, 0.f };
    Vector4f row1{ 0.f, 2 * n / height, 0.f, 0.f };
    Vector4f row3{ 0.f, 0.f, -1.f, 0.f };
    Vector4f row2;
    float Q{ 1.f / (f - n) };

    switch (target_api)
    {
    case EngineAPI::Direct3D12:
    case EngineAPI::Vulkan:
        row2 = invert_depth
            ? Vector4f{ 0.f, 0.f, n*Q, n*f*Q, }
        : Vector4f{ 0.f, 0.f, -f * Q, -n * f*Q };
        break;

    case EngineAPI::Metal:
        LEXGINE_THROW_ERROR("not implemented");
        break;

    case EngineAPI::OpenGL45:
        row2 = Vector4f{ 0.f, 0.f, -(f + n)*Q, -2 * n*f*Q };
        if (invert_depth) row2 *= -1.f;
        break;

    default:
        __assume(0);
    }

    return Matrix4f{ row0, row1, row2, row3 }.transpose();
}

Matrix4f createOrthogonalProjectionMatrix(EngineAPI target_api, float position_x, float position_y, 
    float width, float height, float near_cutoff_distance, float far_cutoff_distance)
{
    float l{ position_x }, r{ position_x + width };
    float b{ position_y + height }, t{ position_y };
    float n{ near_cutoff_distance }, f{ far_cutoff_distance };
    Vector4f row0{ 2.f / (r - l), 0.f, 0.f, (l + r) / (l - r) };
    Vector4f row1{ 0.f, 2.f / (t - b), 0.f, (b + t) / (b - t) };
    Vector4f row3{ 0.f, 0.f, 0.f, 1.f };
    Vector4f row2;

    float Q{ 1.f / (f - n) };

    switch (target_api)
    {
    case EngineAPI::Direct3D12:
    case EngineAPI::Vulkan:
        row2 = Vector4f{ 0.f, 0.f, Q, f * Q };
        break;

    case EngineAPI::Metal:
        LEXGINE_THROW_ERROR("not implemented");
        break;

    case EngineAPI::OpenGL45:
        row2 = Vector4f{ 0.f, 0.f, 2.f * Q, (f + n) * Q };
        break;

    default:
        __assume(0);
    }

    return Matrix4f{ row0, row1, row2, row3 }.transpose();
}

}
