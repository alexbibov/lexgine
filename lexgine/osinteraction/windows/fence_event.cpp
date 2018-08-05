#include "fence_event.h"
#include "lexgine/core/exception.h"

using namespace lexgine::osinteraction::windows;


FenceEvent::FenceEvent(bool is_reset_manually /* = true */) :
    m_event_handle{ CreateEventEx(NULL, NULL, is_reset_manually ? CREATE_EVENT_MANUAL_RESET : 0, EVENT_ALL_ACCESS) },
    m_is_reset_manually{ is_reset_manually }
{
    if (!m_event_handle)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "unable to create WinAPI event handle");
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
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "waiting on WinAPI event has failed");
    }
}

bool FenceEvent::wait(uint32_t milliseconds) const
{
    DWORD result;
    if ((result = WaitForSingleObject(m_event_handle, milliseconds)) == WAIT_FAILED)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "waiting on WinAPI event has failed");
    }

    return result != WAIT_TIMEOUT;
}
