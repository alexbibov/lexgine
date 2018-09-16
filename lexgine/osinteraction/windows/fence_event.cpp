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
    if ((result = WaitForSingleObject(m_event_handle, INFINITE)) != WAIT_OBJECT_0)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "waiting on WinAPI event has failed");
    }
}

bool FenceEvent::wait(uint32_t milliseconds) const
{
    DWORD result = WaitForSingleObject(m_event_handle, milliseconds);
    if (result != WAIT_OBJECT_0 && result != WAIT_TIMEOUT)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "waiting on WinAPI event has failed");
    }

    return result == WAIT_OBJECT_0;
}

void FenceEvent::reset() const
{
    if (!m_is_reset_manually) return;

    if (ResetEvent(m_event_handle) == 0)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "reseting WinAPI event has failed");
    }
}
