#ifndef LEXGINE_CORE_MISC_OPTIONAL_H
#define LEXGINE_CORE_MISC_OPTIONAL_H

#include <stdexcept>
#include <utility>

namespace lexgine { namespace core { namespace misc {

//! Implements simple template wrapper over optional return value @param T.
//! Optional is useful for values that may or may not be present.
//! The wrapper supports implicit conversion to the wrapped type @param T.
//! In addition, if types T and V are implicitly convertible, the same will apply to types Optional<T> and Optional<V>.
template<typename T>
class Optional
{
public:
    Optional() noexcept : m_is_valid{ false } {} //! initializes invalidated wrapper
    Optional(T const& value) : m_value{ value }, m_is_valid{ true } {} //! initializes wrapper containing provided value
    Optional(nullptr_t) noexcept : m_is_valid{ false } {}    //! initializes invalidated wrapper

    Optional(Optional const& other) : m_value{ other.m_value }, m_is_valid{ other.m_is_valid }
    {

    }

    Optional(Optional&& other) : m_value{ std::move(other.m_value) }, m_is_valid{ other.m_is_valid }
    {

    }

    Optional& operator = (Optional const& other)
    {
        if (this == &other) return *this;

        m_value = other.m_value;
        m_is_valid = other.m_is_valid;

        return *this;
    }

    Optional& operator = (Optional&& other)
    {
        if (this == &other) return *this;

        m_value = std::move(other.m_value);
        m_is_valid = other.m_is_valid;

        return *this;
    }

    //! Assigns new value to the wrapper
    Optional& operator = (T const& value)
    {
        m_value = value;
        m_is_valid = true;

        return *this;
    }

    //! Moves value to the wrapper
    Optional& operator = (T&& value)
    {
        m_value = std::move(value);
        m_is_valid = true;

        return *this;
    }

    //! Invalidates wrapper
    Optional& operator = (nullptr_t)
    {
        m_value = T{};
        m_is_valid = false;
    }

    //! Converts wrapper type to the wrapped value
    operator T() const noexcept(false)
    {
        if(m_is_valid) return m_value;
        else throw std::logic_error{ "Attempt to dereference invalid Optional type" };
    }

    //! Converts wrapper type to reference to the wrapped value
    operator T const&() const noexcept(false)
    {
        return const_cast<Optional<T>*>(this)->operator T&();
    }

    //! Converts wrapper type to reference to the wrapped value
    operator T&() noexcept(false)
    {
        if (m_is_valid) return m_value;
        else throw std::logic_error{ "Attempt to dereference invalid Optional type" };
    }

    //! Allows to convert any type to an Optional<> object wrapping it
    template<typename V>
    operator Optional<V>() const
    {
        return Optional<V>{static_cast<V>(m_value)};
    }


    bool isValid() const noexcept { return m_is_valid; }     //! returns 'true' if the wrapper contains a valid value, returns 'false' otherwise


private:
    T m_value;  //!< wrapped value
    bool m_is_valid;    //!< 'true' if the wrapped value is valid, 'false' otherwise
};

}}}

#endif