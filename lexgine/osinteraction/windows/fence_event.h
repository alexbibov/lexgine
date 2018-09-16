#ifndef LEXGINE_OSINTERACTION_WINDOWS_FENCE_EVENT_H

#include <windows.h>

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"

namespace lexgine {namespace osinteraction {namespace windows {

/*! Thin wrapper over Windows system event. The event maintained by the object grants all possible access rights.
  NOTE: this event is not secured by security descriptor and thus will not be inherited by children processes
*/
class FenceEvent final : public core::NamedEntity<core::class_names::FenceEvent>
{
public:
    FenceEvent(bool is_reset_manually = true);

    HANDLE native() const;    //! returns native handle maintained by the operating system
    bool isResetManually() const;    //! returns 'true' if the event has to be reset manually after having been fired
    void wait() const;    //! blocks the calling process until the event is fired

    /*! blocks the calling process until the specified amount of milliseconds has passed or until the event has been fired.
      Returns 'true' if the event has been fired during the specified amount of time. Returns 'false' otherwise.
    */
    bool wait(uint32_t milliseconds) const;

    void reset() const;

private:
    HANDLE m_event_handle;    //! event handle maintained by Windows
    bool m_is_reset_manually;    //! 'true' if the event should be reset manually after having been fired
};

}}}

#define  LEXGINE_OSINTERACTION_WINDOWS_FENCE_EVENT_H
#endif
