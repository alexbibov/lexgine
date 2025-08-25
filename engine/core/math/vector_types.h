#ifndef LEXGINE_CORE_MATH_VECTOR_TYPES_H
#define  LEXGINE_CORE_MATH_VECTOR_TYPES_H

#include <cstdint>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace lexgine::core::math{

template<typename T, uint32_t nelements, glm::qualifier Q>
class vector : public glm::vec<nelements, T, Q>
{
public:
    using glm::vec<nelements, T, Q>::vec;

    vector(glm::vec<nelements, T, Q> const& v):
        glm::vec<nelements, T, Q>{ v }
    {

    }

    T* getRawData()
    {
        T& data = this->x;
        return &data;
    }

    T const* getRawData() const
    {
        return const_cast<vector*>(this)->getRawData();
    }
};

#include "linear_algebra_types.inl"

}

#endif