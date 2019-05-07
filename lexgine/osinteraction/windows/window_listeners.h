// Listeners tailored to window object as implemented in Windows operating system

#ifndef LEXGINE_OSINTERACTION_WINDOWS_WINDOW_LISTENERS_H
#define LEXGINE_OSINTERACTION_WINDOWS_WINDOW_LISTENERS_H

#include <windows.h>

#include "lexgine/core/misc/flags.h"
#include "lexgine/core/math/rectangle.h"

#include "lexgine/osinteraction/listener.h"
#include "lexgine/osinteraction/keyboard.h"

#include "window.h"

namespace lexgine::osinteraction::windows {

//! Listener implementing handling of the basic key hits
class KeyInputListener : public ConcreteListener<WM_KEYDOWN, WM_KEYUP, WM_CHAR, WM_SYSKEYDOWN, WM_SYSKEYUP>
{
public:
    virtual bool keyDown(SystemKey key) = 0;    //! called when a system key is pressed; must be implemented by derived classes. The function should return 'true' on success

    virtual bool keyUp(SystemKey key) = 0;    //! called when a system key is released; must be implemented by derived classes. The function should return 'true' on success

    virtual bool character(wchar_t char_key) = 0;    //! called when a character key is pressed; must be implemented by derived classes. The function should return 'true' on success

    virtual bool systemKeyDown(SystemKey key) = 0;    //! called when the user presses F10 to activate menu or holds down the alt key and then presses one of the system keys.

    virtual bool systemKeyUp(SystemKey key) = 0;    //! called after the user releases a key that was pressed while the alt key was held

protected:
    int64_t process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;
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
    shift = 0x10,
    xbutton1 = 0x20,
    xbutton2 = 0x40
};
}

//! Flags representing virtual control keys that may affect mouse actions when pressed
using ControlKeyFlag = core::misc::Flags<__tag::tagControlKey>;

//! Listener implementing handling of the basic mouse button actions that are present on the most mouse models
class MouseButtonListener : public ConcreteListener<
    WM_LBUTTONDOWN, WM_LBUTTONUP, 
    WM_MBUTTONDOWN, WM_MBUTTONUP, 
    WM_RBUTTONDOWN, WM_RBUTTONUP, 
    WM_XBUTTONDOWN, WM_XBUTTONUP,

    WM_LBUTTONDBLCLK, WM_RBUTTONDBLCLK, WM_XBUTTONDBLCLK,

    WM_MOUSEWHEEL, WM_MOUSEHWHEEL
>
{
public:
    //! Enumerates basic three mouse buttons
    enum class MouseButton
    {
        left,
        middle,
        right,
        x
    };

    //! called when one of the mouse buttons is pressed. The function should return 'true' on success
    virtual bool buttonDown(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) = 0;    
    
    //! called when one of the mouse buttons is released. The function should return 'true' on success
    virtual bool buttonUp(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) = 0;

    //! called when one of the mouse buttons is double-clicked. The function shoudl return 'true' on success
    virtual bool doubleClick(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) = 0;

    //! called when mouse wheel is moved. The function should return 'true' on success
    virtual bool wheelMove(double move_delta, bool is_horizontal_wheel, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) = 0;    

protected:
    int64_t process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;
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
    virtual bool move(uint16_t x, uint16_t y, ControlKeyFlag const& control_key_flag) = 0;

    virtual bool leave_client_area() = 0;    //! called when mouse cursor leaves client area of the listening window. The function should return 'true' on success
    virtual bool enter_client_area() = 0;    //! called when mouse cursor enters client area of the listening window. The function should return 'true' on success

    MouseMoveListener();

protected:
    int64_t process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;

private:
    mutable DWORD m_mouse_tracking_flags;    //! mouse tracking flags (could be "hover" or "leave" or both immediately after initialization)
};



//! Listener that monitors window size change events
class WindowSizeChangeListener : public ConcreteListener<WM_SIZE>
{
public:
    virtual bool minimized() = 0;    //! called when the window has been minimized
    virtual bool maximized(uint16_t new_width, uint16_t new_height) = 0;    //! called when the window has been maximized
    virtual bool size_changed(uint16_t new_width, uint16_t new_height) = 0;    //! called when size of the window has been changed, but neither minimize() nor maximize() does apply

protected:
    int64_t process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;
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
    virtual bool paint(core::math::Rectangle const& update_region) = 0;

protected:
    int64_t process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;
};

}

#endif
