#ifndef LEXGINE_CORE_EXCEPTION_H

#include "entity.h"

namespace lexgine { namespace core {

//! Common exception descriptor. The intended use is for more serious errors, which could rarely be just ignored
class Exception
{
public:
    //! initializes exception object without setting identifier of the object that has caused the exception.
    //! In this case the identifier of the throwing object will be set to a unique value and its string name will
    //! be set to predefined value "lexgine::core::dummy".
    Exception(std::string const& description = "unknown exception");

    template<char const* ThrowingEntityName>
    Exception(NamedEntity<ThrowingEntityName> const& throwing_entity, std::string const& description = "unknown exception") :
        m_thrower_id{ throwing_entity.getId() },
        m_thrower_name{ throwing_entity.getMetaName() },
        m_description{ description }
    {
    }


    EntityID throwingEntityId() const;    //! returns identifier of the entity that caused this exception

    std::string throwingEntityName() const;    //! returns name of the entity that caused this exception

    std::string description() const;    //! returns description of the exception in human-readable form


private:
    EntityID m_thrower_id;    //!< identifier of the object that caused the exception
    std::string m_thrower_name;    //!< name of the object that caused the exception
    std::string m_description;    //!< description of the exception in human-readable form


};

}}

#define LEXGINE_CORE_EXCEPTION_H
#endif
