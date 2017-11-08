#ifndef LEXGINE_CORE_MISC_LOG

#include <iomanip>
#include <sstream>
#include <ostream>
#include <list>
#include <exception>

#include "datetime.h"
#include "misc.h"

namespace lexgine {namespace core {namespace misc {

enum class LogMessageType
{
    information = 0,
    exclamation = 1,
    error = 2
};

//! Implements simple logging system. NOT thread-safe. Create one logging object per thread to avoid racing.
class Log
{
public:
    /*! Initializes the logging system and associates it with provided output stream.
     The time stamps used by the logging system can be adjusted to the time zone via parameter time_zone, which
     can take values between -11 and +12 and parameter is_dts, which determines whether daylight saving time is enabled
     for the selected time zone.
     Note that there can be only one logging object per thread. If this function is invoked twice, the second invocation
     will return reference to the same object as the first one, but provided output stream will be added to the list of
     output streams used by the logging system on this thread.
     */
    static Log const& create(std::ostream& output_logging_stream, std::string const& log_name, int8_t time_zone = 0, bool is_dts = false);

    /*! Retrieves pointer referring to the logging object assigned to the calling thread.
     If no logger has been assigned to the thread, the function will return nullptr.
     */
    static Log const* retrieve();

    //! Attempts to shutdown the logging system and returns 'true' on success or 'false' on failure
    static bool shutdown();

    void out(std::string const& message, LogMessageType message_type) const;	//! logs supplied message out

    DateTime const& getLastEntryTimeStamp() const;	//! returns the time stamp of the last logging entry

    //! Helper to support scoped tabulations
    class scoped_tabulation_helper
    {
    public:
        scoped_tabulation_helper(Log& logger);
        ~scoped_tabulation_helper();

    private:
        Log& m_logger;
    };

private:
    Log(std::ostream& output_logging_stream, int8_t time_zone, bool is_dts);
    Log(Log const&) = delete;
    Log(Log&&) = delete;
    Log& operator=(Log const&) = delete;
    Log& operator=(Log&&) = delete;

    mutable DateTime m_time_stamp;	//!< time stamp of the last logging entry (needed mostly for debugging purposes)
    int16_t m_tabs;    //!< number of tabulations to be added in front of the next logging entry
    std::list<std::ostream*> m_out_streams;	//!< list of output streams used by the logging system
};

}}}


/*! Helper macro that outputs error message to the logger and sets the context object to erroneous state when expr
does not evaluate equal to one of the success codes listed in the variadic argument. In order for this macro to work
properly, supplied context must be inherited from ErrorBehavioral type.
*/
#define LEXGINE_LOG_ERROR_IF_FAILED(context, expr, ...) \
{ \
auto lexgine_error_log_rv = (expr); \
if (!lexgine::core::misc::equalsAny(lexgine_error_log_rv, __VA_ARGS__)) \
{ \
std::stringstream out_message; \
out_message << "Error while executing expression \"" << #expr \
<< "\" in function <" << __FUNCTION__ << "> of module <" << __FILE__ << "> at line " << __LINE__ << ". Error code = 0x" \
<< std::uppercase << std::hex << lexgine_error_log_rv; \
using context_type = std::remove_reference<std::remove_pointer<decltype(context)>::type>::type; \
lexgine::core::misc::dereference<context_type>::resolve(context).raiseError(out_message.str()); \
} \
}

/*!
Unconditionally outputs provided error message to the logger and sets the context object to erroneous state
*/
#define LEXGINE_LOG_ERROR(context, message) \
{ \
std::stringstream out_message; \
out_message << "Error occurred in function <" << __FUNCTION__ << "> of module <" << __FILE__ << "> at line " << __LINE__ \
<< ": " << (message); \
using context_type = std::remove_reference<std::remove_pointer<decltype(context)>::type>::type; \
lexgine::core::misc::dereference<context_type>::resolve(context).raiseError(out_message.str()); \
}

#define LEXGINE_CORE_MISC_LOG
#endif
