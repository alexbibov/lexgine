#ifndef LEXGINE_CORE_MISC_LOG
#define LEXGINE_CORE_MISC_LOG

#include <sstream>
#include <list>
#include <filesystem>

#include "datetime.h"
#include "misc.h"

namespace lexgine::core::misc {

enum class LogMessageType
{
    trace,
    debug,
    information,
    exclamation,
    error,
    critical
};

//! Implements simple logging system. NOT thread-safe. Create one logging object per thread to avoid racing.
class Log
{
public:
    //<! Initializes the logging system and associates it with provided output stream.
    static Log const& create(std::filesystem::path& log_path, std::string const& log_name);

    /*! Retrieves pointer referring to the logging object assigned to the calling thread.
     If no logger has been assigned to the thread, the function will return nullptr.
     */
    static Log const* retrieve();

    //! Attempts to shutdown the logging system and returns 'true' on success or 'false' on failure
    static bool shutdown();

    static void registerMainLogger(Log const* p_logger);	//!< registers main logger

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

    static Log const* m_main_logging_stream;    //!< pointer to the main logger
    static std::mutex m_main_thread_log_mutex;    //!< mutex used to lock main thread logger
};

}


/*! Helper macro that outputs error message to the logger and sets the context object to erroneous state when expr
does not evaluate equal to one of the success codes listed in the variadic argument. In order for this macro to work
properly, supplied context must be inherited from ErrorBehavioral type.
*/
#define LEXGINE_LOG_ERROR_IF_FAILED(context, expr, ...) \
{ \
auto __lexgine_error_log_rv__ = (expr); \
if (!lexgine::core::misc::equalsAny(__lexgine_error_log_rv__, __VA_ARGS__)) \
{ \
std::stringstream out_message; \
out_message << "Error while executing expression \"" << #expr \
<< "\" in function \"" << __FUNCTION__ << "\" of module \"" << __FILE__ << "\" at line " << __LINE__ << ". Error code = 0x" \
<< std::uppercase << std::hex << __lexgine_error_log_rv__; \
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
out_message << "Error occurred in function \"" << __FUNCTION__ << "\" of module \"" << __FILE__ << "\" at line " << __LINE__ \
<< ": " << (message); \
using context_type = std::remove_reference<std::remove_pointer<decltype(context)>::type>::type; \
lexgine::core::misc::dereference<context_type>::resolve(context).raiseError(out_message.str()); \
}

#endif
