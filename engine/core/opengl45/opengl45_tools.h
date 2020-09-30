#ifndef LEXGINE_CORE_MISC_OPENGL45_OPENGL45_TOOLS_H
#define  LEXGINE_CORE_MISC_OPENGL45_OPENGL45_TOOLS_H

#include <GL/glew.h>
#include "constant_converter.h"

namespace lexgine {namespace core {namespace misc {

namespace opengl45 {

//The following template helps to map standard C++ types to OpenGL implementation-dependent types
template<typename T> struct ogl_type_mapper;
template<> struct ogl_type_mapper < bool > { typedef GLboolean ogl_type; };
template<> struct ogl_type_mapper < int > { typedef GLint ogl_type; };
template<> struct ogl_type_mapper < unsigned int > { typedef GLuint ogl_type; };
template<> struct ogl_type_mapper < short > { typedef GLshort ogl_type; };
template<> struct ogl_type_mapper < unsigned short > { typedef GLushort ogl_type; };
template<> struct ogl_type_mapper < float > { typedef GLfloat ogl_type; };
template<> struct ogl_type_mapper < double > { typedef GLdouble ogl_type; };
template<> struct ogl_type_mapper < char > { typedef GLbyte ogl_type; };
template<> struct ogl_type_mapper < unsigned char > { typedef GLubyte ogl_type; };



//The following template is a wrapper used to convert OpenGL data types into corresponding type traits structure
template<typename ogl_data_type, bool special = 0> struct ogl_type_traits;
template<> struct ogl_type_traits < GLbyte > {
    typedef GLbyte ogl_type;
    typedef char iso_c_type;
    static GLenum const ogl_data_type_enum = GL_BYTE;
};

template<> struct ogl_type_traits < GLshort > {
    typedef GLshort ogl_type;
    typedef short iso_c_type;
    static GLenum const ogl_data_type_enum = GL_SHORT;
};

template<> struct ogl_type_traits < GLint > {
    typedef GLint ogl_type;
    typedef int iso_c_type;
    static GLenum const ogl_data_type_enum = GL_INT;
};

template<> struct ogl_type_traits < GLfloat > {
    typedef GLfloat ogl_type;
    typedef float iso_c_type;
    static GLenum const ogl_data_type_enum = GL_FLOAT;
};

template<> struct ogl_type_traits < GLdouble > {
    typedef GLdouble ogl_type;
    typedef double iso_c_type;
    static GLenum const ogl_data_type_enum = GL_DOUBLE;
};

template<> struct ogl_type_traits < GLubyte > {
    typedef GLubyte ogl_type;
    typedef unsigned char iso_c_type;
    static GLenum const ogl_data_type_enum = GL_UNSIGNED_BYTE;
};

template<> struct ogl_type_traits < GLushort > {
    typedef GLushort ogl_type;
    typedef unsigned short iso_c_type;
    static GLenum const ogl_data_type_enum = GL_UNSIGNED_SHORT;
};

template<> struct ogl_type_traits < GLuint > {
    typedef GLuint ogl_type;
    typedef unsigned int iso_c_type;
    static GLenum const ogl_data_type_enum = GL_UNSIGNED_INT;
};

template<> struct ogl_type_traits < GLhalf, true > {
    typedef GLhalf ogl_type;
    typedef unsigned short iso_c_type;
    static GLenum const ogl_data_type_enum = GL_HALF_FLOAT;
};

template<> struct ogl_type_traits < GLfixed, true > {
    typedef GLfixed ogl_type;
    typedef int iso_c_type;
    static GLenum const ogl_data_type_enum = GL_FIXED;
};




//Type wrapper over OpenGL front and back face definition constants
enum class Face : GLenum
{
    FRONT = GL_FRONT,
    BACK = GL_BACK,
    FRONT_AND_BACK = GL_FRONT_AND_BACK
};


//Enumerates possible multi-sample pixel formats supported by the TinyWorld engine
//Not all of these formats may be supported by OpenGL implementation provided by the video driver
enum class MULTISAMPLING_MODE
{
    MULTISAMPLING_NONE = 0,		//no multi-sampling is used (1 sample per pixel)
    MULTISAMPLING_2X = 2,		//2 samples per pixel
    MULTISAMPLING_4X = 4,		//4 samples per pixel
    MULTISAMPLING_8X = 8,		//8 samples per pixel
    MULTISAMPLING_16X = 16		//16 samples per pixel
};

}


template<BlendFactor blend_factor>
struct BlendFactorConverter<EngineAPI::OpenGL45, blend_factor>;   // NOTICE_TO_DEVELOPER: to be implemented

template<BlendOperation blend_op>
struct BlendOperationConverter<EngineAPI::OpenGL45, blend_op>;   // NOTICE_TO_DEVELOPER: to be implemented

template<BlendLogicalOperation blend_logical_op>
struct BlendLogicalOperationConverter<EngineAPI::OpenGL45, blend_logical_op>;   // NOTICE_TO_DEVELOPER: to be implemented

template<FillMode triangle_fill_mode>
struct FillModeConverter<EngineAPI::OpenGL45, triangle_fill_mode>;   // NOTICE_TO_DEVELOPER: to be implemented

template<CullMode cull_mode>
struct CullModeConverter<EngineAPI::OpenGL45, cull_mode>;   // NOTICE_TO_DEVELOPER: to be implemented

template<ConservativeRasterizationMode conservative_rasterization_mode>
struct ConservativeRasterizationModeConverter<EngineAPI::OpenGL45, conservative_rasterization_mode>;   // NOTICE_TO_DEVELOPER: to be implemented

template<ComparisonFunction cmp_fun>
struct ComparisonFunctionConverter<EngineAPI::OpenGL45, cmp_fun>;   // NOTICE_TO_DEVELOPER: to be implemented

template<StencilOperation stencil_op>
struct StencilOperationConverter<EngineAPI::OpenGL45, stencil_op>;   // NOTICE_TO_DEVELOPER: to be implemented

template<PrimitiveTopologyType primitive_topology>
struct PrimitiveTopologyConverter<EngineAPI::OpenGL45, primitive_topology>;    // NOTICE_TO_DEVELOPER: to be implemented

}}}


// GLSL vector types
#include "vector_types.h"

namespace lexgine {namespace core {namespace math {

typedef tagVector4<misc::opengl45::ogl_type_mapper<int>::ogl_type> ivec4;
typedef tagVector4<misc::opengl45::ogl_type_mapper<float>::ogl_type> vec4;
typedef tagVector4<misc::opengl45::ogl_type_mapper<double>::ogl_type> dvec4;
typedef tagVector4<misc::opengl45::ogl_type_mapper<unsigned int>::ogl_type> uvec4;
typedef tagVector4<misc::opengl45::ogl_type_mapper<bool>::ogl_type> bvec4;

typedef tagVector3<misc::opengl45::ogl_type_mapper<int>::ogl_type> ivec3;
typedef tagVector3<misc::opengl45::ogl_type_mapper<float>::ogl_type> vec3;
typedef tagVector3<misc::opengl45::ogl_type_mapper<double>::ogl_type> dvec3;
typedef tagVector3<misc::opengl45::ogl_type_mapper<unsigned int>::ogl_type> uvec3;
typedef tagVector3<misc::opengl45::ogl_type_mapper<bool>::ogl_type> bvec3;

typedef tagVector2<misc::opengl45::ogl_type_mapper<int>::ogl_type> ivec2;
typedef tagVector2<misc::opengl45::ogl_type_mapper<float>::ogl_type> vec2;
typedef tagVector2<misc::opengl45::ogl_type_mapper<double>::ogl_type> dvec2;
typedef tagVector2<misc::opengl45::ogl_type_mapper<unsigned int>::ogl_type> uvec2;
typedef tagVector2<misc::opengl45::ogl_type_mapper<bool>::ogl_type> bvec2;

}}}


// GLSL matrix types
#include "matrix_types.h"
namespace lexgine {namespace core {namespace math {

typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<float>::ogl_type, 4, 4> mat4;
typedef mat4 mat4x4;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<float>::ogl_type, 3, 3> mat3;
typedef mat3 mat3x3;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<float>::ogl_type, 2, 2> mat2;
typedef mat2 mat2x2;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<float>::ogl_type, 4, 3> mat4x3;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<float>::ogl_type, 4, 2> mat4x2;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<float>::ogl_type, 3, 4> mat3x4;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<float>::ogl_type, 2, 4> mat2x4;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<float>::ogl_type, 3, 2> mat3x2;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<float>::ogl_type, 2, 3> mat2x3;

typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<double>::ogl_type, 4, 4> dmat4;
typedef dmat4 dmat4x4;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<double>::ogl_type, 3, 3> dmat3;
typedef dmat3 dmat3x3;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<double>::ogl_type, 2, 2> dmat2;
typedef dmat2 dmat2x2;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<double>::ogl_type, 4, 3> dmat4x3;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<double>::ogl_type, 4, 2> dmat4x2;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<double>::ogl_type, 3, 4> dmat3x4;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<double>::ogl_type, 2, 4> dmat2x4;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<double>::ogl_type, 3, 2> dmat3x2;
typedef shader_matrix_type<misc::opengl45::ogl_type_mapper<double>::ogl_type, 2, 3> dmat2x3;

}}}



#endif