#ifndef LEXGINE_OSINTERACTION_WINDOWS_FENCE_EVENT_H
#define LEXGINE_OSINTERACTION_WINDOWS_FENCE_EVENT_H

#include <functional>

#include <windows.h>

#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"
#include "lexgine/core/misc/optional.h"

namespace lexgine::osinteraction::windows {

/*! Thin wrapper over Windows system event. The event maintained by the object grants all possible access rights.
  NOTE: this event is not secured by security descriptor and thus will not be inherited by children processes
*/
class FenceEvent final : public core::NamedEntity<core::class_names::OSWindows_FenceEvent>
{
public:
    FenceEvent(bool is_reset_manually = true);
    ~FenceEvent();

    HANDLE native() const;    //! returns native handle maintained by the operating system
    bool isResetManually() const;    //! returns 'true' if the event has to be reset manually after having been fired
    void wait() const;    //! blocks the calling process until the event is fired

    /*! spawns a dedicated thread, which will invoke the callback function once the event is set.
     The function returns immediately without blocking. If the callback function was not defined, this function has no effect.
     Parameter @param data_to_pass_to_callback is used to pass arbitrary data to the callback function
     */
    void waitAsync(void* data_to_pass_to_callback) const;  

    //! defines callback function to be invoked when the event is set. Parameter @param data_to_pass_to_callback
    void registerHandler(std::function<void(void*)> const& callback);

    /*! blocks the calling process until the specified amount of milliseconds has passed or until the event has been fired.
      Returns 'true' if the event has been fired during the specified amount of time. Returns 'false' otherwise.
    */
    bool wait(uint32_t milliseconds) const;

    void reset() const;

private:
    using callback_type = std::function<void(void*)>;
    struct callback_message
    {
        callback_type const* callback;
        void* user_data;
    };

private:
    HANDLE m_event_handle;    //!< event handle maintained by Windows
    HANDLE mutable m_wait_handle;    //!< wait handle of the callback function when the latter is defined
    bool m_is_reset_manually;    //!< 'true' if the event should be reset manually after having been fired
    core::misc::Optional<callback_type> m_callback;    //!< callback function, which will be invoked once the event is fired
    callback_message mutable m_callback_message;    //!< structure used for interoperability with windows callback system
};

}

#endif
