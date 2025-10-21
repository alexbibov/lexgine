#ifndef LEXGINE_CORE_MISC_LOG
#define LEXGINE_CORE_MISC_LOG

#include <sstream>
#include <list>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/logger.h>

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
    static Log const& create(std::filesystem::path const& log_path, std::string const& log_name, LogMessageType log_level);

    /*! Retrieves pointer referring to the logging object assigned to the calling thread.
     If no logger has been assigned to the thread, the function will return nullptr.
     */
    static Log const* retrieve();
    static bool shutdown();  // !Attempts to shutdown the logging system and returns 'true' on success or 'false' on failure
    void out(std::string const& message, LogMessageType message_type) const;  //! logs supplied message out
    std::filesystem::path const& logPath() const { return m_log_path; }
    std::string const& logName() const { return m_logger->name(); }

private:
    Log(std::filesystem::path const& log_path) : m_log_path{ log_path } {};
    Log(Log const&) = delete;
    Log(Log&&) = delete;
    Log& operator=(Log const&) = delete;
    Log& operator=(Log&&) = delete;
    
private:
    static std::unique_ptr<Log> m_ptr;
    std::shared_ptr<spdlog::logger> m_logger;
    std::filesystem::path const& m_log_path;
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
