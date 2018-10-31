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
using matrix4f = shader_matrix_type<float, 4, 4>;
using matrix3f = shader_matrix_type<float, 3, 3>;
using matrix2f = shader_matrix_type<float, 2, 2>;
using matrix4x3f = shader_matrix_type<float, 4, 3>;
using matrix4x2f = shader_matrix_type<float, 4, 2>;
using matrix3x4f = shader_matrix_type<float, 3, 4>;
using matrix2x4f = shader_matrix_type<float, 2, 4>;
using matrix3x2f = shader_matrix_type<float, 3, 2>;
using matrix2x3f = shader_matrix_type<float, 2, 3>;

using matrix4d = shader_matrix_type<double, 4, 4>;
using matrix3d = shader_matrix_type<double, 3, 3>;
using matrix2d = shader_matrix_type<double, 2, 2>;
using matrix4x3d = shader_matrix_type<double, 4, 3>;
using matrix4x2d = shader_matrix_type<double, 4, 2>;
using matrix3x4d = shader_matrix_type<double, 3, 4>;
using matrix2x4d = shader_matrix_type<double, 2, 4>;
using matrix3x2d = shader_matrix_type<double, 3, 2>;
using matrix2x3d = shader_matrix_type<double, 2, 3>;
#endif


}

#endif
