#ifndef LEXGINE_CORE_MATH_FWD_H
#define LEXGINE_CORE_MATH_FWD_H

namespace lexgine::core::math {

class Rectangle;
class Box;

template<typename T> struct tagVector2;
template<typename T> struct tagVector3;
template<typename T> struct tagVector4;


#ifndef GENERALIST_VECTOR_TYPES
#define GENERALIST_VECTOR_TYPES
using Vector4i = tagVector4<int>;
using Vector4f = tagVector4<float>;
using Vector4d = tagVector4<double>;
using Vector4u = tagVector4<unsigned int>;
using Vector4b = tagVector4<bool>;

using Vector3i = tagVector3<int>;
using Vector3f = tagVector3<float>;
using Vector3d = tagVector3<double>;
using Vector3u = tagVector3<unsigned int>;
using Vector3b = tagVector3<bool>;

using Vector2i = tagVector2<int>;
using Vector2f = tagVector2<float>;
using Vector2d = tagVector2<double>;
using Vector2u = tagVector2<unsigned int>;
using Vector2b = tagVector2<bool>;
#endif


template<typename T, uint32_t nrows, uint32_t ncolumns>
class shader_matrix_type;

#ifndef GENERALIST_MATRIX_TYPES
#define GENERALIST_MATRIX_TYPES
using Matrix4f = shader_matrix_type<float, 4, 4>;
using Matrix3f = shader_matrix_type<float, 3, 3>;
using Matrix2f = shader_matrix_type<float, 2, 2>;
using Matrix4x3f = shader_matrix_type<float, 4, 3>;
using Matrix4x2f = shader_matrix_type<float, 4, 2>;
using Matrix3x4f = shader_matrix_type<float, 3, 4>;
using Matrix2x4f = shader_matrix_type<float, 2, 4>;
using Matrix3x2f = shader_matrix_type<float, 3, 2>;
using Matrix2x3f = shader_matrix_type<float, 2, 3>;

using Matrix4d = shader_matrix_type<double, 4, 4>;
using Matrix3d = shader_matrix_type<double, 3, 3>;
using Matrix2d = shader_matrix_type<double, 2, 2>;
using Matrix4x3d = shader_matrix_type<double, 4, 3>;
using Matrix4x2d = shader_matrix_type<double, 4, 2>;
using Matrix3x4d = shader_matrix_type<double, 3, 4>;
using Matrix2x4d = shader_matrix_type<double, 2, 4>;
using Matrix3x2d = shader_matrix_type<double, 3, 2>;
using Matrix2x3d = shader_matrix_type<double, 2, 3>;

using Matrix4i = shader_matrix_type<int, 4, 4>;
using Matrix3i = shader_matrix_type<int, 3, 3>;
using Matrix2i = shader_matrix_type<int, 2, 2>;
using Matrix4x3i = shader_matrix_type<int, 4, 3>;
using Matrix4x2i = shader_matrix_type<int, 4, 2>;
using Matrix3x4i = shader_matrix_type<int, 3, 4>;
using Matrix2x4i = shader_matrix_type<int, 2, 4>;
using Matrix3x2i = shader_matrix_type<int, 3, 2>;
using Matrix2x3i = shader_matrix_type<int, 2, 3>;

using Matrix4u = shader_matrix_type<unsigned int, 4, 4>;
using Matrix3u = shader_matrix_type<unsigned int, 3, 3>;
using Matrix2u = shader_matrix_type<unsigned int, 2, 2>;
using Matrix4x3u = shader_matrix_type<unsigned int, 4, 3>;
using Matrix4x2u = shader_matrix_type<unsigned int, 4, 2>;
using Matrix3x4u = shader_matrix_type<unsigned int, 3, 4>;
using Matrix2x4u = shader_matrix_type<unsigned int, 2, 4>;
using Matrix3x2u = shader_matrix_type<unsigned int, 3, 2>;
using Matrix2x3u = shader_matrix_type<unsigned int, 2, 3>;

using Matrix4b = shader_matrix_type<bool, 4, 4>;
using Matrix3b = shader_matrix_type<bool, 3, 3>;
using Matrix2b = shader_matrix_type<bool, 2, 2>;
using Matrix4x3b = shader_matrix_type<bool, 4, 3>;
using Matrix4x2b = shader_matrix_type<bool, 4, 2>;
using Matrix3x4b = shader_matrix_type<bool, 3, 4>;
using Matrix2x4b = shader_matrix_type<bool, 2, 4>;
using Matrix3x2b = shader_matrix_type<bool, 3, 2>;
using Matrix2x3b = shader_matrix_type<bool, 2, 3>;
#endif


}

#endif
