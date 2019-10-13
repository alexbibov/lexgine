#ifndef LEXGINE_CORE_MATH_VECTOR_TYPES_H
#define  LEXGINE_CORE_MATH_VECTOR_TYPES_H

#include <cstdint>

#include "3rd_party/glm/vec2.hpp"
#include "3rd_party/glm/vec3.hpp"
#include "3rd_party/glm/vec4.hpp"

namespace lexgine::core::math{

template<typename T, uint32_t nelements, glm::qualifier Q>
class vector : public glm::vec<nelements, T, Q>
{
public:
    using glm::vec<nelements, T, Q>::vec;

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