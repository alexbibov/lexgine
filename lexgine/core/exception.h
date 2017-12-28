#ifndef LEXGINE_CORE_EXCEPTION_H


#include "entity.h"
#include <exception>
#include <numeric>
#include "misc/misc.h"

namespace lexgine { namespace core {

//! Common exception descriptor. The intended use is for more serious errors, which could rarely be just ignored
class Exception : public std::exception
{
public:
    static long const unknown_line_number = -1;

public:
    template<char const* ThrowingEntityName>
    Exception(NamedEntity<ThrowingEntityName> const& throwing_entity,
        std::string const& description = "unknown exception",
        std::string const& module_name = "<unknown_module>",
        std::string const& function_name = "<unknown_function>",
        long line_number = unknown_line_number) :
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
    long lineNumber() const;    //! returns line number, at which this exception has been thrown

private:
    EntityID const m_thrower_id;    //!< identifier of the object that caused the exception (or of the exception itself if the throwing entity is unknown)
    std::string const m_thrower_name;    //!< name of the object that caused the exception
    std::string const m_module_name;    //!< name of the module that has thrown this exception
    std::string const m_function_name;    //!< name of the function that has thrown this exception
    long const m_line_number;    //!< line number, at which this exception was thrown (unknown_line_number constant stays for unknown)
};

}
}


/*! Checks if expression 'expr' was executed successfully using the list of "success codes" provided
through __VA_ARGS__. If the expression fails, the macro throws exception and sets the calling context
to erroneous state
*/
#define LEXGINE_THROW_ERROR_IF_FAILED(context, expr, ...) \
{ \
auto __lexgine_error_throw_if_failed_rv__ = (expr); \
if (!lexgine::core::misc::equalsAny(__lexgine_error_throw_if_failed_rv__, __VA_ARGS__)) \
{ \
using context_type = std::remove_reference<std::remove_pointer<decltype(context)>::type>::type; \
std::stringstream error_description_string_stream; \
error_description_string_stream << "Named entity <" << lexgine::core::misc::dereference<context_type>::resolve(context).getMetaName() \
<< "> with ID=<" << lexgine::core::misc::dereference<context_type>::resolve(context).getId().toString() \
<< "> has thrown an exception while executing expression \"" << #expr << "\" in function <" << __FUNCTION__ \
<< "> of module <" << __FILE__ << "> at line " << __LINE__ << ". The expression has returned error code 0x" \
<< std::uppercase << std::hex << __lexgine_error_throw_if_failed_rv__; \
std::stringstream short_error_description_string_stream; \
short_error_description_string_stream << "Error while execution expression \"" << #expr \
<< "\": the expression has been executed with error code 0x" << std::uppercase << std::hex << __lexgine_error_throw_if_failed_rv__; \
lexgine::core::misc::dereference<context_type>::resolve(context).raiseError(error_description_string_stream.str()); \
throw Exception{ misc::dereference<context_type>::resolve(context), short_error_description_string_stream.str(), __FILE__, __FUNCTION__, __LINE__ }; \
} \
}

/*!
Sets the context into erroneous state and throws exception using provided description text
*/
#define LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(context, description) \
{ \
using context_type = std::remove_reference<std::remove_pointer<decltype(context)>::type>::type; \
std::stringstream error_string_stream; \
error_string_stream << "Named entity <" << lexgine::core::misc::dereference<context_type>::resolve(context).getMetaName() \
<< "> with ID=<" << lexgine::core::misc::dereference<context_type>::resolve(context).getId().toString() \
<< "> has thrown an exception in function <" << __FUNCTION__ \
<< "> of module <" << __FILE__ << "> at line " << __LINE__ << ": " << (description); \
lexgine::core::misc::dereference<context_type>::resolve(context).raiseError(error_string_stream.str()); \
throw Exception{ misc::dereference<context_type>::resolve(context), description, __FILE__, __FUNCTION__, __LINE__ }; \
}


//! Throws exception using provided description text and "dummy" named entity as the throwing context
#define LEXGINE_THROW_ERROR(description) \
{ \
misc::Log::retrieve()->out(description, misc::LogMessageType::error); \
throw Exception{ lexgine::core::Dummy{}, description, __FILE__, __FUNCTION__, __LINE__ }; \
}


#define LEXGINE_CORE_EXCEPTION_H
#endif
