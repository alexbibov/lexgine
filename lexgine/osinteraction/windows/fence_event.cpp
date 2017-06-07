#include "fence_event.h"
#include "../../core/exception.h"

using namespace lexgine::osinteraction::windows;


FenceEvent::FenceEvent(bool is_reset_manually /* = true */) :
    m_event_handle{ CreateEventEx(NULL, NULL, is_reset_manually ? CREATE_EVENT_MANUAL_RESET : 0, EVENT_ALL_ACCESS) },
    m_is_reset_manually{ is_reset_manually }
{
    if (!m_event_handle)
    {
        std::string err_msg = "unable to create WinAPI event handle";
        logger().out(err_msg);
        raiseError(err_msg);
        throw lexgine::core::Exception{ *this, err_msg };    //! these kind of errors are fatal
    }
}

HANDLE FenceEvent::native() const
{
    return m_event_handle;
}

bool FenceEvent::isResetManually() const
{
    return m_is_reset_manually;
}

void FenceEvent::wait() const
{
    DWORD result;
    if ((result = WaitForSingleObject(m_event_handle, INFINITE)) == WAIT_FAILED)
    {
        std::string err_msg = "waiting on WinAPI event has failed";
        logger().out(err_msg);
        raiseError(err_msg);
        throw lexgine::core::Exception{ *this, err_msg };    // this is a fatal error
    }
}

bool FenceEvent::wait(uint32_t milliseconds) const
{
    DWORD result;
    if ((result = WaitForSingleObject(m_event_handle, milliseconds)) == WAIT_FAILED)
    {
        std::string err_msg = "waiting on WinAPI event has failed";
        logger().out(err_msg);
        raiseError(err_msg);
        throw lexgine::core::Exception{ *this, err_msg };    // this is a fatal error
    }

    return result != WAIT_TIMEOUT;
}
