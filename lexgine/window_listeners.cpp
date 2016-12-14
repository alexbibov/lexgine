#include "window_listeners.h"

using namespace lexgine::osinteraction::windows;

namespace
{

bool keyDownKeyUpSelector(KeyInputListener const& key_input_listener, lexgine::osinteraction::SystemKey key, bool is_key_down)
{
    return is_key_down ? key_input_listener.keyDown(key) : key_input_listener.keyUp(key);
}


ControlKeyFlag transformControlFlag(WPARAM wParamValue)
{
    ControlKeyFlag rv{ 0 };

    if ((wParamValue & MK_CONTROL) == MK_CONTROL) rv.set(ControlKeyFlag::enum_type::ctrl);
    if ((wParamValue & MK_LBUTTON) == MK_LBUTTON) rv.set(ControlKeyFlag::enum_type::left_mouse_button);
    if ((wParamValue & MK_MBUTTON) == MK_MBUTTON) rv.set(ControlKeyFlag::enum_type::middle_mouse_button);
    if ((wParamValue & MK_RBUTTON) == MK_RBUTTON) rv.set(ControlKeyFlag::enum_type::right_mouse_button);
    if ((wParamValue & MK_SHIFT) == MK_SHIFT) rv.set(ControlKeyFlag::enum_type::shift);

    return rv;
}


//! Helper template structure that defines integer type of the same size as the "size of pointers" on the underlying platform
template<unsigned char> struct pointer_sized_int;
template<> struct pointer_sized_int<4> { using type = uint32_t; };
template<> struct pointer_sized_int<8> { using type = uint64_t; };

}


int64_t KeyInputListener::process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
    uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) const
{
    bool isKeyDown = false;
    bool success = false;    // 'true' if the message processing succeeds

    switch (message)
    {
    case WM_KEYDOWN:
        isKeyDown = true;

    case WM_KEYUP:
    {
        bool isExtendedKey = (0x1000000 & lParam) == 0x1000000;

        switch (wParam)
        {
        case VK_TAB:
            success = keyDownKeyUpSelector(*this, SystemKey::tab, isKeyDown);
            break;

        case VK_CAPITAL:
            success = keyDownKeyUpSelector(*this, SystemKey::caps, isKeyDown);
            break;

        case VK_LSHIFT:
            success = keyDownKeyUpSelector(*this, SystemKey::lshift, isKeyDown);
            break;

        case VK_LCONTROL:
            success = keyDownKeyUpSelector(*this, SystemKey::lctrl, isKeyDown);
            break;

        case VK_MENU:
            success = keyDownKeyUpSelector(*this, isExtendedKey ? SystemKey::ralt : SystemKey::lalt, isKeyDown);
            break;

        case VK_BACK:
            success = keyDownKeyUpSelector(*this, SystemKey::backspace, isKeyDown);
            break;

        case VK_RETURN:
            success = keyDownKeyUpSelector(*this, SystemKey::enter, isKeyDown);
            break;

        case VK_RSHIFT:
            success = keyDownKeyUpSelector(*this, SystemKey::rshift, isKeyDown);
            break;

        case VK_RCONTROL:
            success = keyDownKeyUpSelector(*this, SystemKey::rctrl, isKeyDown);
            break;

        case VK_ESCAPE:
            success = keyDownKeyUpSelector(*this, SystemKey::esc, isKeyDown);
            break;

        case VK_F1:
        case VK_F2:
        case VK_F3:
        case VK_F4:
        case VK_F5:
        case VK_F6:
        case VK_F7:
        case VK_F8:
        case VK_F9:
        case VK_F10:
        case VK_F11:
        case VK_F12:
            success = keyDownKeyUpSelector(*this, static_cast<SystemKey>(static_cast<int>(SystemKey::f1) + wParam - VK_F1), isKeyDown);
            break;

        case VK_SNAPSHOT:
            success = keyDownKeyUpSelector(*this, SystemKey::print_screen, isKeyDown);
            break;

        case VK_SCROLL:
            success = keyDownKeyUpSelector(*this, SystemKey::scroll_lock, isKeyDown);
            break;

        case VK_NUMLOCK:
            success = keyDownKeyUpSelector(*this, SystemKey::num_lock, isKeyDown);
            break;

        case VK_PAUSE:
            success = keyDownKeyUpSelector(*this, SystemKey::pause, isKeyDown);
            break;

        case VK_INSERT:
            success = keyDownKeyUpSelector(*this, SystemKey::insert, isKeyDown);
            break;

        case VK_HOME:
            success = keyDownKeyUpSelector(*this, SystemKey::home, isKeyDown);
            break;

        case VK_PRIOR:
            success = keyDownKeyUpSelector(*this, SystemKey::page_up, isKeyDown);
            break;

        case VK_DELETE:
            success = keyDownKeyUpSelector(*this, SystemKey::_delete, isKeyDown);
            break;

        case VK_END:
            success = keyDownKeyUpSelector(*this, SystemKey::end, isKeyDown);
            break;

        case VK_NEXT:
            success = keyDownKeyUpSelector(*this, SystemKey::page_down, isKeyDown);
            break;

        case VK_LEFT:
            success = keyDownKeyUpSelector(*this, SystemKey::arrow_left, isKeyDown);
            break;

        case VK_RIGHT:
            success = keyDownKeyUpSelector(*this, SystemKey::arrow_right, isKeyDown);
            break;

        case VK_DOWN:
            success = keyDownKeyUpSelector(*this, SystemKey::arrow_down, isKeyDown);
            break;

        case VK_UP:
            success = keyDownKeyUpSelector(*this, SystemKey::arrow_up, isKeyDown);
            break;

        case VK_SPACE:
            success = keyDownKeyUpSelector(*this, SystemKey::space, isKeyDown);
            break;

        case VK_NUMPAD0:
        case VK_NUMPAD1:
        case VK_NUMPAD2:
        case VK_NUMPAD3:
        case VK_NUMPAD4:
        case VK_NUMPAD5:
        case VK_NUMPAD6:
        case VK_NUMPAD7:
        case VK_NUMPAD8:
        case VK_NUMPAD9:
            success = keyDownKeyUpSelector(*this, static_cast<SystemKey>(static_cast<int>(SystemKey::num_0) + wParam - VK_NUMPAD0), isKeyDown);
            break;

        case VK_MULTIPLY:
            success = keyDownKeyUpSelector(*this, SystemKey::multiply, isKeyDown);
            break;

        case VK_DIVIDE:
            success = keyDownKeyUpSelector(*this, SystemKey::divide, isKeyDown);
            break;

        case VK_ADD:
            success = keyDownKeyUpSelector(*this, SystemKey::add, isKeyDown);
            break;

        case VK_SUBTRACT:
            success = keyDownKeyUpSelector(*this, SystemKey::subtract, isKeyDown);
            break;

        case VK_DECIMAL:
            success = keyDownKeyUpSelector(*this, SystemKey::decimal, isKeyDown);
            break;

        // keys from 0 to 9
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
            success = keyDownKeyUpSelector(*this, static_cast<SystemKey>(static_cast<int>(SystemKey::_0) + wParam - 0x30), isKeyDown);
            break;

        // keys from A to Z (case insensitive)
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4A:
        case 0x4B:
        case 0x4C:
        case 0x4D:
        case 0x4E:
        case 0x4F:
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
        case 0x58:
        case 0x59:
        case 0x5A:
            success = keyDownKeyUpSelector(*this, static_cast<SystemKey>(static_cast<int>(SystemKey::A) + wParam - 0x41), isKeyDown);
            break;

        default:
            break;
        }

        break;
    }


    case WM_CHAR:
    {
        success = character(static_cast<wchar_t>(wParam));
        break;
    }

    }

    return success ? 0 : -1;
}



int64_t MouseButtonListener::process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
    uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) const
{
    uint16_t x = lParam & 0xFFFF;
    uint16_t y = (lParam & 0xFFFF0000) >> 16;
    ControlKeyFlag control_key_flag = transformControlFlag(static_cast<WPARAM>(wParam));

    MouseButton button;
    bool isButtonPressed = false;

    switch (message)
    {
    case WM_LBUTTONDOWN:
        isButtonPressed = true;

    case WM_LBUTTONUP:
        button = MouseButton::left;
        break;

    case WM_MBUTTONDOWN:
        isButtonPressed = true;
    case WM_MBUTTONUP:
        button = MouseButton::middle;
        break;

    case WM_RBUTTONDOWN:
        isButtonPressed = true;
    case WM_RBUTTONUP:
        button = MouseButton::right;
        break;

    case WM_MOUSEWHEEL:
        return wheelMove(static_cast<double>((wParam & 0xFFFF0000) >> 16) * WHEEL_DELTA, control_key_flag, x, y) ? 0 : -1;
    }

    return (isButtonPressed ? buttonDown(button, control_key_flag, x, y) : buttonUp(button, control_key_flag, x, y)) ? 0 : -1;
}



MouseMoveListener::MouseMoveListener():
    m_mouse_tracking_flags{ TME_HOVER | TME_LEAVE }
{

}


int64_t MouseMoveListener::process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
    uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) const
{
    uint16_t x = lParam & 0xFFFF;
    uint16_t y = (lParam & 0xFFFF0000) >> 16;
    ControlKeyFlag control_key_flag = transformControlFlag(static_cast<WPARAM>(wParam));
    bool success = false;    // 'true' if the message processing succeeds

    TRACKMOUSEEVENT track_mouse_event;
    track_mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
    track_mouse_event.hwndTrack = reinterpret_cast<Window*>(static_cast<pointer_sized_int<sizeof(Window*)>::type>(p_window))->native();
    track_mouse_event.dwHoverTime = HOVER_DEFAULT;
    track_mouse_event.dwFlags = m_mouse_tracking_flags;

    switch (message)
    {
    case WM_MOUSEMOVE:
        success = move(x, y, control_key_flag);
        break;

    case WM_MOUSEHOVER:
        m_mouse_tracking_flags = TME_LEAVE;
        track_mouse_event.dwFlags = m_mouse_tracking_flags;
        success = enter_client_area();
        break;

    case WM_MOUSELEAVE:
        m_mouse_tracking_flags = TME_HOVER;
        track_mouse_event.dwFlags = m_mouse_tracking_flags;
        success = leave_client_area();
        break;
    }

    TrackMouseEvent(&track_mouse_event);

    return success ? 0 : -1;
}



int64_t ClientAreaUpdateListener::process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
    uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) const
{
    core::math::Rectangle rect{ core::math::vector2f{0}, 0, 0 };
    HWND hWnd = reinterpret_cast<Window*>(static_cast<pointer_sized_int<sizeof(Window*)>::type>(p_window))->native();
    RECT windows_rect;
    bool success = false;    // 'true' if the message processing succeeds

    if (GetUpdateRect(hWnd, &windows_rect, FALSE))
    {
        rect.setUpperLeft(core::math::vector2f{ static_cast<float>(windows_rect.left), static_cast<float>(windows_rect.top) });
        rect.setSize(static_cast<float>(windows_rect.right - windows_rect.left), static_cast<float>(windows_rect.top - windows_rect.bottom));

        PAINTSTRUCT ps;
        ps.hdc = GetDC(hWnd);
        ps.fErase = FALSE;
        ps.rcPaint = windows_rect;
        BeginPaint(hWnd, &ps);
        success = paint(rect);
        EndPaint(hWnd, &ps);
    }
    else
    {
        success = paint(rect);
    }

    return success ? 0 : -1;
}

int64_t WindowSizeChangeListener::process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam,
    uint64_t reserved1, uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5) const
{
    uint16_t new_width = lParam & 0xFFFF;
    uint16_t new_height = static_cast<uint16_t>(lParam >> 16);

    switch (wParam)
    {
    case SIZE_MAXIMIZED:
        return maximized(new_width, new_height) ? 0 : -1;

    case SIZE_MINIMIZED:
        return minimized() ? 0 : -1;

    case SIZE_RESTORED:
        return size_changed(new_width, new_height) ? 0 : -1;

    default:
        return -1;
    }
}
