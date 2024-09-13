#define LEXGINE_CLIENT_LIB

#ifndef LEXGINE_CORE_MISC_FLAGS_H
#define LEXGINE_CORE_MISC_FLAGS_H

#include <utility>
#include <string>
#include <type_traits>

namespace lexgine::core::misc {

template<typename BaseFlagsType>
using is_scoped_enum = std::integral_constant<bool,
    std::is_enum<BaseFlagsType>::value && !std::is_convertible<BaseFlagsType, int>::value>;

//! Template class that allows to implement "enumeration" types that could be OR'ed just like normal flags
template<typename BaseFlagsType, typename CompatibleIntegralType = int>
class Flags
{
public:
    using base_values = BaseFlagsType;    //!< base flag values to be OR'ed
    using int_type = CompatibleIntegralType;    //!< base integral type used to store flag values

    //! Default initialization
    Flags() : m_value{ 0 }
    {

    }

    //! Initialization by value
    Flags(CompatibleIntegralType value) : m_value{ value }
    {

    }

    //! Initialization by a base flag value
    Flags(BaseFlagsType flag) : m_value{ static_cast<int_type>(flag) }
    {

    }

    //! Value assignment
    Flags& operator =(CompatibleIntegralType value)
    {
        m_value = value;

        return *this;
    }

    //! Comparison
    bool operator ==(Flags const& other) const
    {
        return m_value == other.m_value;
    }

    //! Conversion to the base type (to enforce type safety only explicit variant is supported)
    explicit operator CompatibleIntegralType() const
    {
        return m_value;
    }

    //! OR's this flag value with provided base flag value and returns the resulting flag value
    Flags operator |(BaseFlagsType const& base_flag) const
    {
        return Flags{ m_value | static_cast<CompatibleIntegralType>(base_flag) };
    }

    Flags& operator |=(BaseFlagsType const& base_flag)
    {
        m_value |= static_cast<CompatibleIntegralType>(base_flag);
        return *this;
    }

    Flags operator^(BaseFlagsType const& base_flag) const
    {
        return Flags { m_value ^ static_cast<CompatibleIntegralType>(base_flag) };
    }

    Flags& operator^=(BaseFlagsType const& base_flag)
    {
        m_value ^= static_cast<CompatibleIntegralType>(base_flag);
        return *this;
    }

    //! Sets given base flag bit. Equivalent to OR'ing this flag bit with the current value of the flags object.
    //! Returns reference to the updated flag object
    Flags& set(BaseFlagsType const& base_flag)
    {
        m_value |= static_cast<CompatibleIntegralType>(base_flag);
        return *this;
    }

    //! Returns 'true' if requested base flag is set in this flag value. Returns 'false' otherwise
    bool isSet(BaseFlagsType const& base_flag) const
    {
        return (m_value & static_cast<CompatibleIntegralType>(base_flag)) == static_cast<CompatibleIntegralType>(base_flag);
    }

    //! Returns encapsulated bitset
    CompatibleIntegralType getValue() const
    {
        return m_value;
    }

    //! Creates flag from provided base flag value
    static Flags create(base_values base_flag)
    {
        return Flags{ base_flag };
    }


private:
    CompatibleIntegralType m_value;
};

}

namespace lexgine
{

template<typename BaseFlagsType,
    typename T0 = std::enable_if<core::misc::is_scoped_enum<BaseFlagsType>::value>::type,
    typename T1 = decltype(BaseFlagsType::_tag_flag_enumeration)>
    core::misc::Flags<BaseFlagsType> operator | (BaseFlagsType a, BaseFlagsType b)
{
    return core::misc::Flags<BaseFlagsType>{a} | b;
}

}

#define BEGIN_FLAGS_DECLARATION(name) enum class _tag##name : int {
#define FLAG(name, value) name = value,
#define END_FLAGS_DECLARATION(name) \
_tag_flag_enumeration = -1 };\
using name = lexgine::core::misc::Flags<_tag##name>

#endif