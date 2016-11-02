// Listeners tailored to window object as implemented in Windows operating system

#ifndef LEXGINE_OSINTERACTION_WINDOWS_WINDOW_LISTENERS_H

#include <windows.h>

#include "listener.h"
#include "keyboard.h"
#include "flags.h"
#include "rectangle.h"
#include "window.h"

namespace lexgine {namespace osinteraction {namespace windows {

//! Listener implementing handling of the basic key hits
class KeyInputListener : public ConcreteListener<WM_KEYDOWN, WM_KEYUP, WM_CHAR>
{
public:
    virtual bool keyDown(SystemKey key) const = 0;    //! called when a system key is pressed; must be implemented by derived classes. The function should return 'true' on success

    virtual bool keyUp(SystemKey key) const = 0;    //! called when a system key is released; must be implemented by derived classes. The function should return 'true' on success

    virtual bool character(wchar_t char_key) const = 0;    //! called when a character key is pressed; must be implemented by derived classes. The function should return 'true' on success

protected:
    int64_t process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) const override;
};



namespace __tag
{
//! Base flags corresponding to the virtual control keys that may affect mouse actions
enum class tagControlKey
{
    ctrl = 1,
    left_mouse_button = 2,
    middle_mouse_button = 4,
    right_mouse_button = 8,
    shift = 16
};
}

//! Flags representing virtual control keys that may affect mouse actions when pressed
using ControlKeyFlag = core::misc::Flags<__tag::tagControlKey>;

//! Listener implementing handling of the basic mouse button actions that are present on the most mouse models
class MouseButtonListener : public ConcreteListener<WM_LBUTTONDOWN, WM_LBUTTONUP, WM_MBUTTONDOWN, WM_MBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP, WM_MOUSEWHEEL>
{
public:
    //! Enumerates basic three mouse buttons
    enum class MouseButton
    {
        left,
        middle,
        right
    };

    virtual bool buttonDown(MouseButton button, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) const = 0;    //! called when one of the mouse buttons is pressed. The function should return 'true' on success
    virtual bool buttonUp(MouseButton button, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) const = 0;    //! called when one of the mouse buttons is released. The function should return 'true' on success
    virtual bool wheelMove(double move_delta, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) const = 0;    //! called when mouse wheel is moved. The function should return 'true' on success

protected:
    int64_t process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) const override;
};


//! Listener implementing handling of the basic mouse move actions
class MouseMoveListener : public ConcreteListener<WM_MOUSEMOVE, WM_MOUSELEAVE, WM_MOUSEHOVER>
{
public:
    /*! called when mouse cursor moves over client area of the listening window.
      Here x and y are coordinates of the cursor, and control_key_flag determines
      which virtual control keys were pressed when the action was fired.
      The function should return 'true' on success and 'false' on failure
    */
    virtual bool move(uint16_t x, uint16_t y, ControlKeyFlag const& control_key_flag) const = 0;

    virtual bool leave_client_area() const = 0;    //! called when mouse cursor leaves client area of the listening window. The function should return 'true' on success
    virtual bool enter_client_area() const = 0;    //! called when mouse cursor enters client area of the listening window. The function should return 'true' on success

    MouseMoveListener();

protected:
    int64_t process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) const override;

private:
    mutable DWORD m_mouse_tracking_flags;    //! mouse tracking flags (could be "hover" or "leave" or both immediately after initialization)
};



//! Listener that monitors window size change events
class WindowSizeChangeListener : public ConcreteListener<WM_SIZE>
{
public:
    virtual bool minimized() const = 0;    //! called when the window has been minimized
    virtual bool maximized(uint16_t new_width, uint16_t new_height) const = 0;    //! called when the window has been maximized
    virtual bool size_changed(uint16_t new_width, uint16_t new_height) const = 0;    //! called when size of the window has been changed, but neither minimize() nor maximize() does apply

protected:
    int64_t process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) const override;
};


/*! Listener implementing handling of the system event, which notifies application that the client
  area of the listening window can now be updated
*/
class ClientAreaUpdateListener : public ConcreteListener<WM_PAINT>
{
public:
    /*! Called when all or part of client area of the listening window is invalidated.
      Note that @param update_region may have zero dimensions, in which case it should be assumed that the whole client area should be updated.
      The function should return 'true' on success and 'false' on failure.
    */
    virtual bool paint(core::math::Rectangle const& update_region) const = 0;

protected:
    int64_t process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) const override;
};

}}}

#define LEXGINE_OSINTERACTION_WINDOWS_WINDOW_LISTENERS_H
#endif
