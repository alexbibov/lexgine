#ifndef LEXGINE_CORE_MISC_FLAGS_H

#include <utility>
#include <string>

namespace lexgine {namespace core {namespace misc {

//! Template class that allows to implement "enumeration" types that could be OR'ed just like normal flags
template<typename BaseFlagType, typename BaseIntegralType = int>
class Flags
{
public:
    using enum_type = BaseFlagType;    //!< base flag values to be OR'ed
    using base_int_type = BaseIntegralType;    //!< base integral type used to store flag values

    //! Default initialization
    Flags() : m_value{ 0 }
    {

    }

    //! Initialization by value
    Flags(BaseIntegralType value) : m_value{ value }
    {

    }

    //! Initialization by a base flag value
    Flags(BaseFlagType flag) : m_value{ static_cast<base_int_type>(flag) }
    {

    }

    //! Value assignment
    Flags& operator =(BaseIntegralType value)
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
    explicit operator BaseIntegralType() const
    {
        return m_value;
    }

    //! OR's this flag value with provided base flag value and returns the resulting flag value
    Flags operator |(BaseFlagType const& base_flag) const
    {
        return Flags{ m_value | static_cast<BaseIntegralType>(base_flag) };
    }

    Flags& operator |=(BaseFlagType const& base_flag)
    {
        m_value |= static_cast<BaseIntegralType>(base_flag);
        return *this;
    }

    //! Sets given base flag bit. Equivalent to OR'ing this flag bit with the current value of the flags object.
    //! Returns reference to the updated flag object
    Flags& set(BaseFlagType const& base_flag)
    {
        m_value |= static_cast<BaseIntegralType>(base_flag);
        return *this;
    }

    //! Returns 'true' if requested base flag is set in this flag value. Returns 'false' otherwise
    bool isSet(BaseFlagType const& base_flag) const
    {
        return (m_value & static_cast<BaseIntegralType>(base_flag)) == static_cast<BaseIntegralType>(base_flag);
    }

    //! Returns encapsulated bitset
    BaseIntegralType getValue() const
    {
        return m_value;
    }

    //! Creates flag from provided base flag value
    static Flags create(enum_type base_flag)
    {
        return Flags{ base_flag };
    }


private:
    BaseIntegralType m_value;
};


}}}

#define LEXGINE_CORE_MISC_FLAGS_H
#endif