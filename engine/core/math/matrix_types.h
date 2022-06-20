#ifndef LEXGINE_CORE_MATH_MATRIX_TYPES_H
#define LEXGINE_CORE_MATH_MATRIX_TYPES_H

#include "3rd_party/glm/matrix.hpp"
#include "3rd_party/glm/gtc/matrix_integer.hpp"


namespace lexgine::core::math {

template<typename T, uint32_t nrows, uint32_t ncolumns, glm::qualifier Q>
class matrix : public glm::mat<ncolumns, nrows, T, Q>
{
public:
    using glm::mat<ncolumns, nrows, T, Q>::mat;

    matrix(glm::mat<ncolumns, nrows, T, Q> const& m)
        : glm::mat<ncolumns, nrows, T, Q>{ m }
    {

    }

public:
    typename glm::mat<ncolumns, nrows, T, Q>::transpose_type transpose() const
    {
        return glm::transpose(*this);
    }

    T* getRawData()
    {
        typename glm::mat<ncolumns, nrows, T, Q>::col_type& data = (*this)[0];
        return reinterpret_cast<T*>(&data);
    }

    T const* getRawData() const
    {
        return const_cast<matrix*>(this)->getRawData();
    }
};

template<typename T, uint32_t order, glm::qualifier Q>
class matrix<T, order, order, Q> : public glm::mat<order, order, T, Q>
{
public:
    using glm::mat<order, order, T, Q>::mat;

    matrix(glm::mat<order, order, T, Q> const& m)
        : glm::mat<order, order, T, Q>{ m }
    {

    }

public:
    typename glm::mat<order, order, T, Q>::transpose_type transpose() const
    {
        return glm::transpose(*this);
    }

    T* getRawData()
    {
        typename glm::mat<order, order, T, Q>::col_type& data = (*this)[0];
        return reinterpret_cast<T*>(&data);
    }

    T const* getRawData() const
    {
        return const_cast<matrix*>(this)->getRawData();
    }

    T determinant() const
    {
        return glm::determinant(*this);
    }
};


#include "linear_algebra_types.inl"

}

#endif