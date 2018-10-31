#ifndef LEXGINE_CORE_MATH_VECTOR_TYPES_H
#define  LEXGINE_CORE_MATH_VECTOR_TYPES_H

#include <initializer_list>



// NOTE: add AVX optimizations!


// NOTE: there is small mistake in design of these classes, which leads to necessity to repeat vector-vector and matrix-vector
// operation overriding. Possible way to alleviate this problem is to inherit vector types from a common type, which comprises
// common operations, and design each concrete vector type as an inherited template specialization

namespace lexgine::core::math {

template<typename T> struct tagVector2;
template<typename T> struct tagVector3;
template<typename T> struct tagVector4;

template<typename T>
struct tagVector4 {
private:
    // helper adapter converting bool to int
    template<typename T> struct bool_to_int { using type = T; };
    template<> struct bool_to_int<bool> { using type = int; };

    mutable T data[4];		//!< this variable is only used to return data stored in vector as an array. This becomes handy for uniform assignments.

public:
    typedef T value_type;
    static char const dimension = 4;

    T x;
    T y;
    T z;
    T w;

    tagVector4(T x, T y, T z, T w): x{ x }, y{ y }, z{ z }, w{ w } {}
    tagVector4(T x, T y, T z): x{ x }, y{ y }, z{ z }, w{ 0 } {}
    tagVector4(T x, T y): x{ x }, y{ y }, z{ 0 }, w{ 0 } {}
    tagVector4(T x): x{ x }, y{ x }, z{ x }, w{ x } {}
    tagVector4(): x{}, y{}, z{}, w{} {}

    tagVector4(tagVector3<T> const& v3, T w): x{ v3.x }, y{ v3.y }, z{ v3.z }, w{ w } {}
    tagVector4(T x, tagVector3<T> const& v3): x{ x }, y{ v3.x }, z{ v3.y }, w{ v3.z } {}
    tagVector4(tagVector2<T>& v2_1, tagVector2<T> const& v2_2): x{ v2_1.x }, y{ v2_1.y }, z{ v2_2.x }, w{ v2_2.y } {}
    tagVector4(tagVector2<T> const& v2, T z, T w): x{ v2.x }, y{ v2.y }, z{ z }, w{ w } {}
    tagVector4(T x, tagVector2<T> const& v2, T w): x{ x }, y{ v2.x }, z{ v2.y }, w{ w } {}
    tagVector4(T x, T y, tagVector2<T> const& v2): x{ x }, y{ y }, z{ v2.x }, w{ v2.y } {}

    tagVector4(tagVector4 const& other): 
        x{ other.x }, 
        y{ other.y },
        z{ other.z }, 
        w{ other.w }
    {

    }

    // returns data stored in vector packed into an array
    T const* getDataAsArray() const
    {
        data[0] = x; data[1] = y; data[2] = z; data[3] = w;
        return data;
    }

    // returns norm of contained vector
    decltype(1.0f / static_cast<typename bool_to_int<T>::type>(T{ 1 })) norm() const
    {
        return std::sqrt(x*x + y*y + z*z + w*w);
    }

    // returns normalized version of contained vector
    tagVector4<decltype(1.0f / static_cast<typename bool_to_int<T>::type>(T{ 1 }))> get_normalized() const
    {
        using result_type = decltype(1.0f / T{ 1 });
        result_type norm_factor = norm();
        return tagVector4<result_type> {x, y, z, w} / norm_factor;
    }

    // returns dot product of two vectors
    T dot_product(tagVector4 const& other) const
    {
        return x*other.x + y*other.y + z*other.z + w*other.w;
    }

    // component wise multiplication by -1
    tagVector4<T> operator -() const
    {
        return tagVector4<T>{-x, -y, -z, -w};
    }

    // element access for read-only via indexing operator
    T const& operator[](int const index) const
    {
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        case 3:
            return w;
        default:
            throw(std::range_error("Out of range: vec4 only allows element access for indexes from 0 to 3"));
        }
    }

    // element access via indexing operator for read-write
    T& operator[](int const index)
    {
        return const_cast<T&>(const_cast<tagVector4 const*>(this)->operator [](index));
    }

    template<typename P>
    bool operator==(tagVector4<P> const& other) const
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    tagVector4& operator=(tagVector4 const& other)
    {
        if (this == &other)
            return *this;

        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }

    // Conversion between vectors with different base value types
    template<typename P>
    explicit operator tagVector4<P>() const
    {
        return tagVector4<P>{static_cast<P>(x), static_cast<P>(y), static_cast<P>(z), static_cast<P>(w)};
    }

    template<typename P>
    auto operator+(tagVector4<P> const& other) const -> tagVector4<decltype(T{} + P{})>
    {
        return tagVector4<decltype(T{}+P{})>{x + other.x, y + other.y, z + other.z, w + other.w};
    }

    template<typename P>
    tagVector4<T>& operator+=(tagVector4<P> const& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;

        return *this;
    }

    template<typename P>
    auto operator-(tagVector4<P> const& other) const -> tagVector4<decltype(T{}-P{})>
    {
        return tagVector4<decltype(T{} -P{}) > {x - other.x, y - other.y, z - other.z, w - other.w};
    }

    template<typename P>
    tagVector4<T>& operator-=(tagVector4<P> const& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;

        return *this;
    }

    template<typename P>
    auto operator *(tagVector4<P> const& other) const -> tagVector4<decltype(T{}*P{})>
    {
        return tagVector4<decltype(T{}*P{})>{x * other.x, y * other.y, z * other.z, w * other.w};
    }

    template<typename P>
    tagVector4<T>& operator*=(tagVector4<P> const& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;

        return *this;
    }

    template<typename P>
    auto operator /(tagVector4<P> const& other) const -> tagVector4<decltype(T{ 1 } / P{ 1 })>
    {
        return tagVector4<decltype(T{ 1 } / P{ 1 })>{x / other.x, y / other.y, z / other.z, w / other.w};
    }

    template<typename P>
    tagVector4<T>& operator/=(tagVector4<P> const& other)
    {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        w /= other.w;

        return *this;
    }

    template<typename P>
    auto operator *(P alpha) const -> tagVector4<decltype(T{}*P{})>
    {
        return tagVector4<decltype(T{}*P{})>{x*alpha, y*alpha, z*alpha, w*alpha};
    }

    template<typename P>
    tagVector4<T>& operator*=(P alpha)
    {
        x *= alpha;
        y *= alpha;
        z *= alpha;
        w *= alpha;

        return *this;
    }

    template<typename P>
    auto operator /(P alpha) const -> tagVector4<decltype(T{ 1 } / P{ 1 })>
    {
        return tagVector4<decltype(T{ 1 } / P{ 1 }) > {x / alpha, y / alpha, z / alpha, w / alpha};
    }

    template<typename P>
    tagVector4<T>& operator /=(P alpha)
    {
        x /= alpha;
        y /= alpha;
        z /= alpha;
        w /= alpha;

        return *this;
    }
};

template<typename T>
tagVector4<decltype(float{}*T{}) > operator*(float alpha, tagVector4<T> const& vector)
{
    return tagVector4 < decltype(float{}*T{}) > {alpha*vector.x, alpha*vector.y, alpha*vector.z, alpha*vector.w};
}

template<typename T>
tagVector4<decltype(double{}*T{}) > operator*(double alpha, tagVector4<T> const& vector)
{
    return tagVector4 < decltype(double{}*T{}) > {alpha*vector.x, alpha*vector.y, alpha*vector.z, alpha*vector.w};
}



template<typename T>
struct tagVector3
{
private:
    // helper adapter converting bool to int
    template<typename T> struct bool_to_int { using type = T; };
    template<> struct bool_to_int<bool> { using type = int; };

    mutable T data[3];		//this variable is only used to return data stored in vector as an array. This becomes handy for uniform assignments.

public:
    typedef T value_type;
    static char const dimension = 3;

    T x;
    T y;
    T z;

    tagVector3(T x, T y, T z) : x{ x }, y{ y }, z{ z } {}
    tagVector3(T x, T y) : x{ x }, y{ y }, z{ 0 } {}
    tagVector3(T x) : x{ x }, y{ x }, z{ x } {}
    tagVector3() : x{}, y{}, z{} {}

    tagVector3(tagVector2<T> const& v2, T z) : x{ v2.x }, y{ v2.y }, z{ z } {}
    tagVector3(T x, tagVector2<T> const& v2) : x{ x }, y{ v2.x }, z{ v2.y } {}

    //Copy constructor
    tagVector3(tagVector3 const& other) : x(other.x), y(other.y), z(other.z)
    {

    }

    //returns data stored in vector represented by an array
    T const * getDataAsArray() const
    {
        data[0] = x; data[1] = y; data[2] = z;
        return data;
    }

    //returns norm of contained vector
    decltype(1.0f / static_cast<typename bool_to_int<T>::type>(T{ 1 })) norm() const
    {
        return std::sqrt(x*x + y*y + z*z);
    }

    //returns normalized version of contained vector
    tagVector3<decltype(1.0f / static_cast<typename bool_to_int<T>::type>(T{ 1 })) > get_normalized() const
    {
        typedef decltype(1.0f / T{ 1 }) result_type;
        result_type norm_factor = norm();
        return tagVector3 < result_type > {x, y, z} / norm_factor;
    }

    //returns dot product of two vectors
    T dot_product(tagVector3 const& other) const
    {
        return x*other.x + y*other.y + z*other.z;
    }

    //returns cross product of two vectors
    tagVector3 cross_product(tagVector3 const& other) const
    {
        return tagVector3<T>{y*other.z - z*other.y, -x*other.z + z*other.x, x*other.y - y*other.x};
    }

    //converts vec3 defined in 3D Euclidean space to the corresponding vec4 defined in 4D Homogeneous space
    template<typename P>
    explicit operator tagVector4<P>() const
    {
        return tagVector4<P>{static_cast<P>(x), static_cast<P>(y), static_cast<P>(z), P{ 1 }};
    }

    //Conversion between templates with different value types
    template<typename P>
    explicit operator tagVector3<P>() const
    {
        return tagVector3<P>{static_cast<P>(x), static_cast<P>(y), static_cast<P>(z)};
    }

    //component wise multiplication by -1
    tagVector3<T> operator -() const
    {
        return tagVector3<T>{-x, -y, -z};
    }

    //element access for read-only via indexing operator
    T const& operator[](int const index) const
    {
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        default:
            throw(std::range_error("Out of range: vec3 only allows element access for indexes from 0 to 2"));
        }
    }

    //element access via indexing operator for read-write
    T& operator[](int const index)
    {
        return const_cast<T&>(const_cast<tagVector3 const *>(this)->operator [](index));
    }

    //Template comparison operator
    template<typename P>
    bool operator==(tagVector3<P> const& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    //Copy assignment operator
    tagVector3& operator=(tagVector3 const& other)
    {
        if (this == &other)
            return *this;

        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }


    //vector addition
    template<typename P>
    auto operator +(tagVector3<P> const& other) const->tagVector3<decltype(T{} +P{}) >
    {
        return tagVector3<decltype(T{} +P{}) > {x + other.x, y + other.y, z + other.z};
    }

    //Template addition-assignment operator
    template<typename P>
    tagVector3<T>& operator+=(tagVector3<P> const& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;

        return *this;
    }

    //vector subtraction
    template<typename P>
    auto operator -(tagVector3<P> const& other) const->tagVector3<decltype(T{} -P{}) >
    {
        return tagVector3<decltype(T{} -P{}) > {x - other.x, y - other.y, z - other.z};
    }

    //Template subtraction-assignment operator
    template<typename P>
    tagVector3<T>& operator-=(tagVector3<P> const& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;

        return *this;
    }

    //component-wise vector multiplication
    template<typename P>
    auto operator *(tagVector3<P> const& other) const->tagVector3<decltype(T{} *P{}) >
    {
        return tagVector3<decltype(T{} *P{}) > {x * other.x, y * other.y, z * other.z};
    }

    //Template component-wise vector multiplication-assignment operator
    template<typename P>
    tagVector3<T>& operator*=(tagVector3<P> const& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;

        return *this;
    }

    //component-wise vector division
    template<typename P>
    auto operator /(tagVector3<P> const& other) const->tagVector3 < decltype(T{ 1 } / P{ 1 }) >
    {
        return tagVector3 < decltype(T{ 1 } / P{ 1 }) > {x / other.x, y / other.y, z / other.z};
    }

    //Template component-wise vector division-assignment operator
    template<typename P>
    tagVector3<T>& operator/=(tagVector3<P> const& other)
    {
        x /= other.x;
        y /= other.y;
        z /= other.z;

        return *this;
    }

    //multiplies vector by scalar
    template<typename P>
    auto operator *(P alpha) const->tagVector3 < decltype(T{}*P{}) >
    {
        return tagVector3 < decltype(T{}*P{}) > {x*alpha, y*alpha, z*alpha};
    }

    //Template vector-scalar multiplication-assignment operator
    template<typename P>
    tagVector3<T>& operator*=(P alpha)
    {
        x *= alpha;
        y *= alpha;
        z *= alpha;

        return *this;
    }

    //divides vector by scalar
    template<typename P>
    auto operator /(P alpha) const->tagVector3 < decltype(T{ 1 } / P{ 1 }) >
    {
        return tagVector3 < decltype(T{ 1 } / P{ 1 }) > {x / alpha, y / alpha, z / alpha};
    }

    //Template vector-scalar division-assignment operator
    template<typename P>
    tagVector3<T>& operator/=(P alpha)
    {
        x /= alpha;
        y /= alpha;
        z /= alpha;
        return *this;
    }
};

//multiplies scalar by vector
template<typename T>
tagVector3<decltype(float{}*T{}) > operator*(float alpha, tagVector3<T> const& vector)
{
    return tagVector3 < decltype(float{}*T{}) > {alpha*vector.x, alpha*vector.y, alpha*vector.z};
}

template<typename T>
tagVector3<decltype(double{}*T{}) > operator*(double alpha, tagVector3<T> const& vector)
{
    return tagVector3 < decltype(double{}*T{}) > {alpha*vector.x, alpha*vector.y, alpha*vector.z};
}



template<typename T>
struct tagVector2
{
private:
    // helper adapter converting bool to int
    template<typename T> struct bool_to_int { using type = T; };
    template<> struct bool_to_int<bool> { using type = int; };

    mutable T data[2];		//this variable is only used to return data stored in vector as an array. This becomes handy for uniform assignments.

public:
    typedef T value_type;
    static char const dimension = 2;

    T x;
    T y;

    tagVector2(T x, T y) : x{ x }, y{ y } {}
    tagVector2(T x) : x{ x }, y{ x } {}
    tagVector2() : x{}, y{} {}

    //Copy constructor
    tagVector2(tagVector2 const& other) : x(other.x), y(other.y)
    {

    }

    //returns data stored in vector represented by an array
    T const * getDataAsArray() const
    {
        data[0] = x; data[1] = y;
        return data;
    }

    //returns norm of contained vector
    decltype(1.0f / static_cast<typename bool_to_int<T>::type>(T{ 1 })) norm() const
    {
        return std::sqrt(x*x + y*y);
    }

    //returns normalized version of contained vector
    tagVector2<decltype(1.0f / static_cast<typename bool_to_int<T>::type>(T{ 1 })) > get_normalized() const
    {
        typedef decltype(1.0f / T{ 1 }) result_type;
        result_type norm_factor = norm();
        return tagVector2 < result_type > {x, y} / norm_factor;
    }

    //returns dot product of two vectors
    T dot_product(tagVector2 const& other) const
    {
        return x*other.x + y*other.y;
    }

    //converts vec2 to vec3
    template<typename P>
    explicit operator tagVector3<P>() const
    {
        return tagVector3<P>{static_cast<P>(x), static_cast<P>(y), P{ 0 }};
    }

    //converts vec2 represented in 2D Euclidean space to the corresponding vec4 represented in 4D Homogeneous space
    template<typename P>
    explicit operator tagVector4<P>() const
    {
        return tagVector4<P>{static_cast<P>(x), static_cast<P>(y), static_cast<P>(0), P{ 1 }};
    }

    //Conversion between templates with different value types
    template<typename P>
    explicit operator tagVector2<P>() const
    {
        return tagVector2<P>{static_cast<P>(x), static_cast<P>(y)};
    }

    //component wise multiplication by -1
    tagVector2<T> operator -() const
    {
        return tagVector2<T>{-x, -y};
    }

    //element access for read-only via indexing operator
    T const& operator[](int const index) const
    {
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        default:
            throw(std::range_error("Out of range: vec2 only allows element access for indexes from 0 to 1"));
        }
    }

    //element access via indexing operator for read-write
    T& operator[](int const index)
    {
        return const_cast<T&>(const_cast<tagVector2 const *>(this)->operator [](index));
    }

    //Template comparison operator
    template<typename P>
    bool operator==(tagVector2<P> const& other) const
    {
        return x == other.x && y == other.y;
    }

    //Copy assignment operator
    tagVector2& operator=(tagVector2 const& other)
    {
        if (this == &other)
            return *this;

        x = other.x;
        y = other.y;
        return *this;
    }

    //vector addition
    template<typename P>
    auto operator +(tagVector2<P> const& other) const->tagVector2<decltype(T{} +P{}) >
    {
        return tagVector2<decltype(T{} +P{}) > {x + other.x, y + other.y};
    }

    //Template addition-assignment operator
    template<typename P>
    tagVector2<T>& operator+=(tagVector2<P> const& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    //vector subtraction
    template<typename P>
    auto operator -(tagVector2<P> const& other) const->tagVector2<decltype(T{} -P{}) >
    {
        return tagVector2<decltype(T{} -P{}) > {x - other.x, y - other.y};
    }

    //Template subtraction-assignment operator
    template<typename P>
    tagVector2<T>& operator-=(tagVector2<P> const& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    //component-wise vector multiplication
    template<typename P>
    auto operator *(tagVector2<P> const& other) const->tagVector2<decltype(T{} *P{}) >
    {
        return tagVector2<decltype(T{} *P{}) > {x * other.x, y * other.y};
    }

    //Template component-wise vector multiplication-assignment operator
    template<typename P>
    tagVector2<T>& operator*=(tagVector2<P> const& other)
    {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    //component-wise vector division
    template<typename P>
    auto operator /(tagVector2<P> const& other) const->tagVector2 < decltype(T{ 1 } / P{ 1 }) >
    {
        return tagVector2 < decltype(T{ 1 } / P{ 1 }) > {x / other.x, y / other.y};
    }

    //Template component-wise vector division-assignment operator
    template<typename P>
    tagVector2<T>& operator/=(tagVector2<P> const& other)
    {
        x /= other.x;
        y /= other.y;
        return *this;
    }

    //multiplies vector by scalar
    template<typename P>
    auto operator *(P alpha) const->tagVector2<decltype(T{} *P{}) >
    {
        return tagVector2 < decltype(T{} *P{}) > {x*alpha, y*alpha};
    }

    //Template vector-scalar multiplication-assignment operator
    template<typename P>
    tagVector2<T>& operator*=(P alpha)
    {
        x *= alpha;
        y *= alpha;
        return *this;
    }

    //divides vector by scalar
    template<typename P>
    auto operator /(P alpha) const->tagVector2 < decltype(T{ 1 } / P{ 1 }) >
    {
        return tagVector2 < decltype(T{ 1 } / P{ 1 }) > {x / alpha, y / alpha};
    }

    //Template vector-scalar division-assignment operator
    template<typename P>
    tagVector2<T>& operator/=(P alpha)
    {
        x /= alpha;
        y /= alpha;
        return *this;
    }
};

//multiplies scalar by vector
template<typename T>
tagVector2<decltype(float{}*T{}) > operator*(float alpha, tagVector2<T> const& vector)
{
    return tagVector2 < decltype(float{}*T{}) > {alpha*vector.x, alpha*vector.y};
}

template<typename T>
tagVector2<decltype(double{}*T{}) > operator*(double alpha, tagVector2<T> const& vector)
{
    return tagVector2 < decltype(double{}*T{}) > {alpha*vector.x, alpha*vector.y};
}


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

}

#endif