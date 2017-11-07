#ifndef LEXGINE_CORE_EXCEPTION_H

#include "entity.h"
#include <exception>
#include "misc/misc.h"

namespace lexgine { namespace core {

//! Common exception descriptor. The intended use is for more serious errors, which could rarely be just ignored
class Exception : public std::exception
{
public:
    static uint32_t const unknown_line_number = 0xFFFFFFFF;

public:
    template<char const* ThrowingEntityName>
    Exception(NamedEntity<ThrowingEntityName> const& throwing_entity,
        std::string const& description = "unknown exception",
        std::string const& module_name = "<unknown_module>",
        std::string const& function_name = "<unknown_function>",
        uint32_t line_number = unknown_line_number) :
        std::exception{ description.c_str() },
        m_thrower_id{ throwing_entity.getId() },
        m_thrower_name{ throwing_entity.getMetaName() },
        m_module_name{ module_name },
        m_function_name{ function_name },
        m_line_number{ line_number }
    {
        
    }

    EntityID throwingEntityId() const;    //! returns identifier of the entity that caused this exception
    std::string const& throwingEntityName() const;    //! returns name of the entity that caused this exception
    std::string const& throwingModuleName() const;    //! returns name of the module that has thrown this exception
    std::string const& throwingFunctionName() const;    //! returns name of the function that has thrown this exception
    uint32_t lineNumber() const;    //! returns line number, at which this exception has been thrown

private:
    EntityID const m_thrower_id;    //!< identifier of the object that caused the exception (or of the exception itself if the throwing entity is unknown)
    std::string const m_thrower_name;    //!< name of the object that caused the exception
    std::string const m_module_name;    //!< name of the module that has thrown this exception
    std::string const m_function_name;    //!< name of the function that has thrown this exception
    uint32_t const m_line_number;    //!< line number, at which this exception was thrown (0xFFFFFFFF stays for unknown)
};

}
}


#define LEXGINE_THROW_ERROR_IF_FAILED(context, expr, ...) \
{ \
auto rv = (expr); \
if (!lexgine::core::misc::equalsAny(rv, __VA_ARGS__)) \
{ \
using context_type = std::remove_reference<std::remove_pointer<decltype(context)>::type>::type; \
std::stringstream error_description_string_stream; \
error_description_string_stream << "Named entity <" << lexgine::core::misc::dereference<context_type>::resolve(context).getMetaName() << "> with ID=<" \
<< lexgine::core::misc::dereference<context_type>::resolve(context).getId().toString() << "> has thrown an exception while executing expression \"" << #expr \
<< "\" in function <" << __FUNCTION__ << "> of module <" << __FILE__ << "> at line " << __LINE__ << ". The expression has returned error code 0x" << std::uppercase << std::hex << rv; \
std::stringstream short_error_description_string_stream; \
short_error_description_string_stream << "Error while execution expression \"" << #expr << "\": the expression has been executed with error code 0x" << std::uppercase << std::hex << rv; \
lexgine::core::misc::dereference<context_type>::resolve(context).raiseError(error_description_string_stream.str()); \
throw Exception{ misc::dereference<context_type>::resolve(context), short_error_description_string_stream.str(), __FILE__, __FUNCTION__, __LINE__ }; \
} \
}

#define LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(context, description) \
{ \
using context_type = std::remove_reference<std::remove_pointer<decltype(context)>::type>::type; \
std::stringstream error_string_stream; \
error_string_stream << "Named entity <" << lexgine::core::misc::dereference<context_type>::resolve(context).getMetaName() << "> with ID=<" \
<< lexgine::core::misc::dereference<context_type>::resolve(context).getId().toString() \
<< "> has thrown an exception in function <" << __FUNCTION__ \
<< "> of module <" << __FILE__ << "> at line " << __LINE__ << ": " << description; \
lexgine::core::misc::dereference<context_type>::resolve(context).raiseError(error_string_stream.str()); \
throw Exception{ misc::dereference<context_type>::resolve(context), description, __FILE__, __FUNCTION__, __LINE__ }; \
}

#define LEXGINE_THROW_ERROR(description) throw Exception{ lexgine::core::Dummy{}, (description), __FILE__, __FUNCTION__, __LINE__ };


#define LEXGINE_CORE_EXCEPTION_H
#endif
