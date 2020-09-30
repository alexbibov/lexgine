#ifndef LEXGINE_CORE_MATH_LINEAR_ALGEBRA_VECTOR_TYPES
#define LEXGINE_CORE_MATH_LINEAR_ALGEBRA_VECTOR_TYPES

template<typename T, uint32_t nelements, glm::qualifier Q>
class vector;


using Vector4i = vector<int, 4, glm::defaultp>;
using Vector4f = vector<float, 4, glm::defaultp>;
using Vector4d = vector<double, 4, glm::defaultp>;
using Vector4u = vector<unsigned int, 4, glm::defaultp>;
using Vector4b = vector<bool, 4, glm::defaultp>;

using Vector3i = vector<int, 3, glm::defaultp>;
using Vector3f = vector<float, 3, glm::defaultp>;
using Vector3d = vector<double, 3, glm::defaultp>;
using Vector3u = vector<unsigned int, 3, glm::defaultp>;
using Vector3b = vector<bool, 3, glm::defaultp>;

using Vector2i = vector<int, 2, glm::defaultp>;
using Vector2f = vector<float, 2, glm::defaultp>;
using Vector2d = vector<double, 2, glm::defaultp>;
using Vector2u = vector<unsigned int, 2, glm::defaultp>;
using Vector2b = vector<bool, 2, glm::defaultp>;

#endif


#ifndef LEXGINE_CORE_MATH_LINEAR_ALGEBRA_MATRIX_TYPES
#define LEXGINE_CORE_MATH_LINEAR_ALGEBRA_MATRIX_TYPES

template<typename T, uint32_t nrows, uint32_t ncolumns, glm::qualifier Q>
class matrix;


using Matrix4f = matrix<float, 4, 4, glm::defaultp>;
using Matrix3f = matrix<float, 3, 3, glm::defaultp>;
using Matrix2f = matrix<float, 2, 2, glm::defaultp>;
using Matrix4x3f = matrix<float, 4, 3, glm::defaultp>;
using Matrix4x2f = matrix<float, 4, 2, glm::defaultp>;
using Matrix3x4f = matrix<float, 3, 4, glm::defaultp>;
using Matrix2x4f = matrix<float, 2, 4, glm::defaultp>;
using Matrix3x2f = matrix<float, 3, 2, glm::defaultp>;
using Matrix2x3f = matrix<float, 2, 3, glm::defaultp>;

using Matrix4d = matrix<double, 4, 4, glm::defaultp>;
using Matrix3d = matrix<double, 3, 3, glm::defaultp>;
using Matrix2d = matrix<double, 2, 2, glm::defaultp>;
using Matrix4x3d = matrix<double, 4, 3, glm::defaultp>;
using Matrix4x2d = matrix<double, 4, 2, glm::defaultp>;
using Matrix3x4d = matrix<double, 3, 4, glm::defaultp>;
using Matrix2x4d = matrix<double, 2, 4, glm::defaultp>;
using Matrix3x2d = matrix<double, 3, 2, glm::defaultp>;
using Matrix2x3d = matrix<double, 2, 3, glm::defaultp>;

#if defined(LEXGINE_MATH_INTEGER_MATRIX_PRECISION_HIGH)
using Matrix4i = matrix<int, 4, 4, glm::highp>;
using Matrix3i = matrix<int, 3, 3, glm::highp>;
using Matrix2i = matrix<int, 2, 2, glm::highp>;
using Matrix4x3i = matrix<int, 4, 3, glm::highp>;
using Matrix4x2i = matrix<int, 4, 2, glm::highp>;
using Matrix3x4i = matrix<int, 3, 4, glm::highp>;
using Matrix2x4i = matrix<int, 2, 4, glm::highp>;
using Matrix3x2i = matrix<int, 3, 2, glm::highp>;
using Matrix2x3i = matrix<int, 2, 3, glm::highp>;

using Matrix4u = matrix<unsigned int, 4, 4, glm::highp>;
using Matrix3u = matrix<unsigned int, 3, 3, glm::highp>;
using Matrix2u = matrix<unsigned int, 2, 2, glm::highp>;
using Matrix4x3u = matrix<unsigned int, 4, 3, glm::highp>;
using Matrix4x2u = matrix<unsigned int, 4, 2, glm::highp>;
using Matrix3x4u = matrix<unsigned int, 3, 4, glm::highp>;
using Matrix2x4u = matrix<unsigned int, 2, 4, glm::highp>;
using Matrix3x2u = matrix<unsigned int, 3, 2, glm::highp>;
using Matrix2x3u = matrix<unsigned int, 2, 3, glm::highp>;
#elif defined(LEXGINE_MATH_INTERGER_MATRIX_PRECISION_LOW)
using Matrix4i = matrix<int, 4, 4, glm::lowp>;
using Matrix3i = matrix<int, 3, 3, glm::lowp>;
using Matrix2i = matrix<int, 2, 2, glm::lowp>;
using Matrix4x3i = matrix<int, 4, 3, glm::lowp>;
using Matrix4x2i = matrix<int, 4, 2, glm::lowp>;
using Matrix3x4i = matrix<int, 3, 4, glm::lowp>;
using Matrix2x4i = matrix<int, 2, 4, glm::lowp>;
using Matrix3x2i = matrix<int, 3, 2, glm::lowp>;
using Matrix2x3i = matrix<int, 2, 3, glm::lowp>;

using Matrix4u = matrix<unsigned int, 4, 4, glm::lowp>;
using Matrix3u = matrix<unsigned int, 3, 3, glm::lowp>;
using Matrix2u = matrix<unsigned int, 2, 2, glm::lowp>;
using Matrix4x3u = matrix<unsigned int, 4, 3, glm::lowp>;
using Matrix4x2u = matrix<unsigned int, 4, 2, glm::lowp>;
using Matrix3x4u = matrix<unsigned int, 3, 4, glm::lowp>;
using Matrix2x4u = matrix<unsigned int, 2, 4, glm::lowp>;
using Matrix3x2u = matrix<unsigned int, 3, 2, glm::lowp>;
using Matrix2x3u = matrix<unsigned int, 2, 3, glm::lowp>;
#else
using Matrix4i = matrix<int, 4, 4, glm::mediump>;
using Matrix3i = matrix<int, 3, 3, glm::mediump>;
using Matrix2i = matrix<int, 2, 2, glm::mediump>;
using Matrix4x3i = matrix<int, 4, 3, glm::mediump>;
using Matrix4x2i = matrix<int, 4, 2, glm::mediump>;
using Matrix3x4i = matrix<int, 3, 4, glm::mediump>;
using Matrix2x4i = matrix<int, 2, 4, glm::mediump>;
using Matrix3x2i = matrix<int, 3, 2, glm::mediump>;
using Matrix2x3i = matrix<int, 2, 3, glm::mediump>;

using Matrix4u = matrix<unsigned int, 4, 4, glm::mediump>;
using Matrix3u = matrix<unsigned int, 3, 3, glm::mediump>;
using Matrix2u = matrix<unsigned int, 2, 2, glm::mediump>;
using Matrix4x3u = matrix<unsigned int, 4, 3, glm::mediump>;
using Matrix4x2u = matrix<unsigned int, 4, 2, glm::mediump>;
using Matrix3x4u = matrix<unsigned int, 3, 4, glm::mediump>;
using Matrix2x4u = matrix<unsigned int, 2, 4, glm::mediump>;
using Matrix3x2u = matrix<unsigned int, 3, 2, glm::mediump>;
using Matrix2x3u = matrix<unsigned int, 2, 3, glm::mediump>;
#endif

#endif