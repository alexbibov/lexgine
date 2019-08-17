#include "fence_event.h"
#include "lexgine/core/exception.h"

using namespace lexgine::osinteraction::windows;

namespace {

std::string convertWindowsErrorCodeToString(DWORD error_code)
{
    LPWSTR p_message;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, error_code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        reinterpret_cast<LPWSTR>(&p_message), 256U, NULL);
    std::string error_message = lexgine::core::misc::wstringToAsciiString(std::wstring{ p_message });
    LocalFree(p_message);
    return error_message;
}

}


FenceEvent::FenceEvent(bool is_reset_manually /* = true */) :
    m_event_handle{ CreateEventEx(NULL, NULL, is_reset_manually ? CREATE_EVENT_MANUAL_RESET : 0, EVENT_ALL_ACCESS) },
    m_is_reset_manually{ is_reset_manually }
{
    if (!m_event_handle)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "unable to create WinAPI event handle");
    }
}

FenceEvent::FenceEvent(FenceEvent&& other)
    : m_event_handle{ other.m_event_handle }
    , m_is_reset_manually{ other.m_is_reset_manually }
{
    other.m_event_handle = 0;
}

FenceEvent::~FenceEvent()
{
    if (!m_event_handle) return;

    DWORD error_code;

    if (m_callback.isValid())
    {
        if (!UnregisterWait(m_wait_handle))
        {
            error_code = GetLastError();
            if (error_code != ERROR_IO_PENDING)
            {
                LEXGINE_LOG_ERROR(this, convertWindowsErrorCodeToString(error_code));
            }
        }
    }

    if (!CloseHandle(m_event_handle))
    {
        error_code = GetLastError();
        LEXGINE_LOG_ERROR(this, convertWindowsErrorCodeToString(error_code));
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

void FenceEvent::waitAsync(void* data_to_pass_to_callback) const
{
    if (m_callback.isValid())
    {
        m_callback_message.callback = &static_cast<callback_type const&>(m_callback);
        m_callback_message.user_data = data_to_pass_to_callback;
        if (!RegisterWaitForSingleObject(&m_wait_handle, m_event_handle,
            [](PVOID data, BOOLEAN)
            {
                callback_message* p_callback_message = reinterpret_cast<callback_message*>(data);
                (*p_callback_message->callback)(p_callback_message->user_data);
            },
            &m_callback_message,
                INFINITE, WT_EXECUTEDEFAULT))
        {
            DWORD error_code = GetLastError();
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, convertWindowsErrorCodeToString(error_code));
        }
    }
}

void FenceEvent::registerHandler(std::function<void(void*)> const& callback)
{
    if (m_callback.isValid())
    {
        if (!UnregisterWait(m_wait_handle))
        {
            DWORD error_code = GetLastError();
            if (error_code != ERROR_IO_PENDING)
            {
                LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, convertWindowsErrorCodeToString(error_code));
            }
        }
    }

    m_callback = callback;
}

bool FenceEvent::wait(uint32_t milliseconds) const
{
    DWORD result = WaitForSingleObject(m_event_handle, milliseconds);
    if (result != WAIT_OBJECT_0 && result != WAIT_TIMEOUT)
    {
        DWORD error_code = GetLastError();
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, convertWindowsErrorCodeToString(error_code));
    }

    return result == WAIT_OBJECT_0;
}

void FenceEvent::reset() const
{
    if (!m_is_reset_manually) return;

    if (!ResetEvent(m_event_handle))
    {
        DWORD error_code = GetLastError();
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, convertWindowsErrorCodeToString(error_code));
    }
}
