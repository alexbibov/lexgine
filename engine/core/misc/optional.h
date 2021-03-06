#ifndef LEXGINE_CORE_MISC_OPTIONAL_H
#define LEXGINE_CORE_MISC_OPTIONAL_H

#include <stdexcept>
#include <utility>

namespace lexgine::core::misc{

//! Implements nullable type wrapper over provided type T
template<typename T>
class Optional final
{
public:
    using value_type = T;

public:
    Optional() noexcept : m_is_valid{ false } {}    //! initializes invalidated wrapper

    //! initializes wrapper containing provided value
    Optional(T const& value) :
        m_is_valid{ true }
    {
        new(m_value) T{ value };
    }

    // ! initializes wrapper by moving provided value into it
    Optional(T&& value) :
        m_is_valid{ true }
    {
        new(m_value) T{ std::move(value) };
    }

    Optional(Optional const& other) :
        m_is_valid{ other.m_is_valid }
    {
        if (other.m_is_valid)
            new(m_value) T{ *reinterpret_cast<T const*>(other.m_value) };
    }

    Optional(Optional&& other) :
        m_is_valid{ other.m_is_valid }
    {
        if (other.m_is_valid)
            new(m_value) T{ std::move(*reinterpret_cast<T const*>(other.m_value)) };
    }

    //! constructs wrapped type "in-place" without copy overhead
    template<typename A, typename... Args,
        typename = std::enable_if<std::is_constructible<T, A, Args...>::value
        && !(std::is_same<std::decay<A>::type, T>::value && sizeof...(Args) == 0)>::value>
    explicit Optional(A&& a0, Args&&... args) :
        m_is_valid{ true }
    {
        new(m_value) T(std::forward<A>(a0), std::forward<Args>(args)...);
    }

    ~Optional()
    {
        invalidate();
    }

    Optional& operator=(Optional const& other)
    {
        if (this == &other) return *this;

        if (m_is_valid && other.m_is_valid)
            *reinterpret_cast<T*>(m_value) = *reinterpret_cast<T const*>(other.m_value);
        else if (!other.m_is_valid)
        {
            invalidate();
        }
        else
        {
            new(m_value) T{ *reinterpret_cast<T const*>(other.m_value) };
            m_is_valid = true;
        }

        return *this;
    }


    //! Assigns new value to the wrapper
    Optional& operator=(T const& value)
    {
        if (m_is_valid)
        {
            *reinterpret_cast<T*>(m_value) = value;
        }
        else
        {
            new(m_value) T{ value };
        }

        m_is_valid = true;

        return *this;
    }

    //! Moves value to the wrapper
    Optional& operator=(T&& value)
    {
        if (m_is_valid)
        {
            *reinterpret_cast<T*>(m_value) = std::move(value);
        }
        else
        {
            new(m_value) T{ std::move(value) };
        }

        m_is_valid = true;

        return *this;
    }

    //! Converts wrapper type to constant reference to the wrapped value
    operator T const&() const noexcept(false)
    {
        return const_cast<Optional<T>*>(this)->operator T&();
    }

    //! Converts wrapper type to the wrapped value
    operator T() const noexcept(false)
    {
        if (m_is_valid) return *reinterpret_cast<T*>(m_value);
        else throw std::logic_error{ "Attempt to dereference invalid Optional wrapper" };
    }

    //! Converts wrapper type to reference to the wrapped value
    operator T&() noexcept(false)
    {
        if (m_is_valid) return *reinterpret_cast<T*>(m_value);
        else throw std::logic_error{ "Attempt to dereference invalid Optional wrapper" };
    }

    //! Implements conversion between wrappers corresponding to conversion between compatible types
    template<typename V>
    operator Optional<V>() const
    {
        return Optional<V>{static_cast<V const>(*reinterpret_cast<T const*>(m_value))};
    }


    bool isValid() const noexcept { return m_is_valid; }     //! returns 'true' if the wrapper contains a valid value, returns 'false' otherwise

    //! invalidates the wrapper if it was valid; otherwise has no effect
    void invalidate()
    {
        if (m_is_valid)
        {
            destruction_t::destruct(*reinterpret_cast<T*>(m_value));
            m_is_valid = false;
        }
    }


private:
    template<typename T, bool>
    struct destruct_wrapped_type
    {
        static inline void destruct(T const&) {};
    };

    template<typename T> struct destruct_wrapped_type<T, true>
    {
        static inline void destruct(T const& val)
        {
            val.~T();
        }
    };

    using destruction_t = destruct_wrapped_type<T, std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value>;

private:
    char m_value[sizeof(T)];  //!< wrapped value buffer
    bool m_is_valid;    //!< 'true' if the wrapped value is valid, 'false' otherwise
};

template<typename T> 
Optional<T> makeEmptyOptional()
{
    return Optional<T>{};
}

template<typename T, typename... Args> 
Optional<T> makeOptional(Args&&... args)
{
    return Optional<T>{std::forward<Args>(args)...};
}

}

#endif