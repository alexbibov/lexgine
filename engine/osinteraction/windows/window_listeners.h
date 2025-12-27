// Listeners tailored to window object as implemented in Windows operating system

#ifndef LEXGINE_OSINTERACTION_WINDOWS_WINDOW_LISTENERS_H
#define LEXGINE_OSINTERACTION_WINDOWS_WINDOW_LISTENERS_H

#include <windows.h>

#include "engine/core/misc/flags.h"
#include "engine/core/math/rectangle.h"

#include "engine/osinteraction/listener.h"
#include "engine/osinteraction/keyboard.h"
#include "engine/osinteraction/mouse.h"

#include "window.h"

namespace lexgine::osinteraction::windows {

//! Listener implementing handling of the basic key hits
class KeyInputListener : public ConcreteListener<WM_KEYDOWN, WM_KEYUP, WM_CHAR, WM_SYSKEYDOWN, WM_SYSKEYUP>
{
public:
    virtual bool keyDown(SystemKey key) { return true; }  //! called when a system key is pressed; The function returns 'true' on success

    virtual bool keyUp(SystemKey key) { return true; }  //! called when a system key is released; The function returns 'true' on success

    virtual bool character(uint64_t char_key_code) { return true; }  //! called when a character key is pressed; The should returns 'true' on success

    virtual bool systemKeyDown(SystemKey key) { return true; }  //! called when the user presses F10 to activate menu or holds down the alt key and then presses one of the system keys.

    virtual bool systemKeyUp(SystemKey key) { return true; }  //! called after the user releases a key that was pressed while the alt key was held

protected:
    bool process_message(uint64_t message, uint64_t p_window, uint64_t wparam, uint64_t lparam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;
};


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
    //! called when one of the mouse buttons is pressed. The function should return 'true' on success
    virtual bool buttonDown(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) { return true; }

    //! called when one of the mouse buttons is released. The function should return 'true' on success
    virtual bool buttonUp(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) { return true; }

    //! called when one of the mouse buttons is double-clicked. The function should return 'true' on success
    virtual bool doubleClick(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) { return true; }

    //! called when mouse wheel is moved. The function should return 'true' on success
    virtual bool wheelMove(double move_delta, bool is_horizontal_wheel, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) { return true; }

protected:
    bool process_message(uint64_t message, uint64_t p_window, uint64_t wparam, uint64_t lparam, uint64_t reserved1,
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
    virtual bool move(uint16_t x, uint16_t y, ControlKeyFlag const& control_key_flag) { return true; }

    virtual bool leave_client_area() { return true; }    //! called when mouse cursor leaves client area of the listening window. The function should return 'true' on success
    virtual bool enter_client_area() { return true; }    //! called when mouse cursor enters client area of the listening window. The function should return 'true' on success

    MouseMoveListener();

protected:
    bool process_message(uint64_t message, uint64_t p_window, uint64_t wparam, uint64_t lparam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;

private:
    mutable DWORD m_mouse_tracking_flags;    //! mouse tracking flags (could be "hover" or "leave" or both immediately after initialization)
};



//! Listener that monitors window size change events
class WindowSizeChangeListener : public ConcreteListener<WM_SIZE>
{
public:
    virtual bool minimized() { return true; }    //! called when the window has been minimized
    virtual bool maximized(uint16_t new_width, uint16_t new_height) { return true; }    //! called when the window has been maximized
    virtual bool size_changed(uint16_t new_width, uint16_t new_height) { return true; }    //! called when size of the window has been changed, but neither minimize() nor maximize() does apply

protected:
    bool process_message(uint64_t message, uint64_t p_window, uint64_t wparam, uint64_t lparam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;
};


/*! Listener implementing handling of the system events, which notifies application that the client
  area of the listening window can now be updated. This listener also handles the cursor properties of the window.
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
    bool process_message(uint64_t message, uint64_t p_window, uint64_t wparam, uint64_t lparam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;
};


//! Listener implementing handling of the events posted to window when it is about to set its cursor properties
class CursorUpdateListener : public ConcreteListener<WM_SETCURSOR>
{
public:
    virtual bool setCursor(uint64_t wparam, uint64_t lparam) = 0;

protected:
    bool process_message(uint64_t message, uint64_t p_window, uint64_t wparam, uint64_t lparam, uint64_t reserved1,
        uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;
};

// Listener implementing handling of focus events
class FocusUpdateListener : public ConcreteListener<WM_SETFOCUS, WM_KILLFOCUS>
{
public:
    virtual bool setFocus(uint64_t param) { return true; }
    virtual bool killFocus(uint64_t param) { return true; }

protected:
    bool process_message(uint64_t message, uint64_t p_window, uint64_t wparam, uint64_t lparam, uint64_t reserved1,
		uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;
};

// Listener implementing language switch
class InputLanguageChangeListener : public ConcreteListener<WM_INPUTLANGCHANGE>
{
public:
    virtual bool inputLanguageChanged() = 0;

protected:
    bool process_message(uint64_t message, uint64_t p_window, uint64_t wparam, uint64_t lparam, uint64_t reserved1,
	uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) override;
};

}

#endif
