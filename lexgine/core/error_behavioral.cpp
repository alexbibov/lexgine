#include <cassert>

#include "error_behavioral.h"

using namespace lexgine::core;



void ErrorBehavioral::registerErrorCallback(std::function<void(std::string const & err_msg)> error_callback)
{
    m_error_callback = error_callback;
}

void ErrorBehavioral::addLoggingStream(std::ostream& log_stream)
{
   misc::Log::create(log_stream);
}

bool ErrorBehavioral::resetErrorState()
{
    bool rv = m_error_state;
    m_error_state = false;
    return rv;
}

bool ErrorBehavioral::getErrorState() const
{
    return m_error_state;
}

char const* ErrorBehavioral::getErrorString() const
{
    return m_error_string.c_str();
}




ErrorBehavioral::ErrorBehavioral() :
    m_error_state{ false },
    m_error_string{ "" },
    m_error_callback{ [](std::string const&) {} },
    m_p_logger{ misc::Log::retrieve() }
{
    assert(m_p_logger);
}

ErrorBehavioral::~ErrorBehavioral()
{

}

void ErrorBehavioral::raiseError(std::string const& error_message) const
{
    m_error_state = true;
    m_error_string = error_message;
    m_error_callback(m_error_string);

    m_p_logger->out(error_message, lexgine::core::misc::LogMessageType::error);
}

misc::Log const& ErrorBehavioral::logger() const { return *m_p_logger; }