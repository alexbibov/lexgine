#include <cassert>

#include "error_behavioral.h"

using namespace lexgine::core;



void ErrorBehavioral::registerErrorCallback(std::function<void(std::string const & err_msg)> error_callback)
{
    m_error_callback = error_callback;
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
    m_error_callback{ [](std::string const&) {} }
{
    
}

ErrorBehavioral::~ErrorBehavioral()
{

}

void ErrorBehavioral::raiseError(std::string const& error_message) const
{
    m_error_state = true;
    m_error_string = error_message;
    m_error_callback(m_error_string);

    logger().out(error_message, lexgine::core::misc::LogMessageType::error);
}

misc::Log& ErrorBehavioral::logger() const 
{ 
    misc::Log* p_logger = misc::Log::retrieve();
    assert(p_logger);

    return *p_logger;
}