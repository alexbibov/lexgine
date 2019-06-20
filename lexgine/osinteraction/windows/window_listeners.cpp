#include "window_listeners.h"

using namespace lexgine::osinteraction;
using namespace lexgine::osinteraction::windows;

namespace
{

bool keyDownKeyUpSelector(KeyInputListener& key_input_listener, lexgine::osinteraction::SystemKey key, bool is_key_down)
{
    return is_key_down ? key_input_listener.keyDown(key) : key_input_listener.keyUp(key);
}


ControlKeyFlag transformControlFlag(WPARAM wParam)
{
    ControlKeyFlag rv{ 0 };

    if ((wParam & MK_CONTROL) == MK_CONTROL) rv.set(ControlKeyFlag::base_values::ctrl);
    if ((wParam & MK_LBUTTON) == MK_LBUTTON) rv.set(ControlKeyFlag::base_values::left_mouse_button);
    if ((wParam & MK_MBUTTON) == MK_MBUTTON) rv.set(ControlKeyFlag::base_values::middle_mouse_button);
    if ((wParam & MK_RBUTTON) == MK_RBUTTON) rv.set(ControlKeyFlag::base_values::right_mouse_button);
    if ((wParam & MK_SHIFT) == MK_SHIFT) rv.set(ControlKeyFlag::base_values::shift);
    if ((wParam & MK_XBUTTON1) == MK_XBUTTON1) rv.set(ControlKeyFlag::base_values::xbutton1);
    if ((wParam & MK_XBUTTON2) == MK_XBUTTON2) rv.set(ControlKeyFlag::base_values::xbutton2);

    return rv;
}

SystemKey transformSystemKey(uint64_t wParam, uint64_t lParam)
{
    bool isExtendedKey = (0x1000000 & lParam) != 0;

    switch (wParam)
    {
    case VK_TAB:        return SystemKey::tab;
    case VK_CAPITAL:    return SystemKey::caps;
    case VK_LSHIFT:     return SystemKey::lshift;
    case VK_LCONTROL:   return SystemKey::lctrl;
    case VK_MENU:       return isExtendedKey ? SystemKey::ralt : SystemKey::lalt;
    case VK_BACK:       return SystemKey::backspace;
    case VK_RETURN:     return SystemKey::enter;
    case VK_RSHIFT:     return SystemKey::rshift;
    case VK_RCONTROL:   return SystemKey::rctrl;
    case VK_ESCAPE:     return SystemKey::esc;

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
        return static_cast<SystemKey>(static_cast<int>(SystemKey::f1) + wParam - VK_F1);

    case VK_SNAPSHOT:   return SystemKey::print_screen;
    case VK_SCROLL:     return SystemKey::scroll_lock;
    case VK_NUMLOCK:    return SystemKey::num_lock;
    case VK_PAUSE:      return SystemKey::pause;
    case VK_INSERT:     return SystemKey::insert;
    case VK_HOME:       return SystemKey::home;
    case VK_PRIOR:      return SystemKey::page_up;
    case VK_DELETE:     return SystemKey::_delete;
    case VK_END:        return SystemKey::end;
    case VK_NEXT:       return SystemKey::page_down;
    case VK_LEFT:       return SystemKey::arrow_left;
    case VK_RIGHT:      return SystemKey::arrow_right;
    case VK_DOWN:       return SystemKey::arrow_down;
    case VK_UP:         return SystemKey::arrow_up;
    case VK_SPACE:      return SystemKey::space;

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
        return static_cast<SystemKey>(static_cast<int>(SystemKey::num_0) + wParam - VK_NUMPAD0);

    case VK_MULTIPLY:   return SystemKey::multiply;
    case VK_DIVIDE:     return SystemKey::divide;
    case VK_ADD:        return SystemKey::add;
    case VK_SUBTRACT:   return SystemKey::subtract;
    case VK_DECIMAL:    return SystemKey::decimal;


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
        return static_cast<SystemKey>(static_cast<int>(SystemKey::_0) + wParam - 0x30);


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
        return static_cast<SystemKey>(static_cast<int>(SystemKey::A) + wParam - 0x41);

    case VK_LBUTTON: return SystemKey::mouse_left_button;
    case VK_RBUTTON: return SystemKey::mouse_right_button;
    case VK_MBUTTON: return SystemKey::mouse_middle_button;
    case VK_XBUTTON1: return SystemKey::mouse_x_button_1;
    case VK_XBUTTON2: return SystemKey::mouse_x_button_2;

    default:
        return SystemKey::unknown;
    }
}


//! Helper template structure that defines integer type of the same size as the "size of pointers" on the underlying platform
template<unsigned char> struct pointer_sized_int;
template<> struct pointer_sized_int<4> { using type = uint32_t; };
template<> struct pointer_sized_int<8> { using type = uint64_t; };

}


int64_t KeyInputListener::process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
    uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5)
{
    bool is_key_down = false;
    bool success = false;    // 'true' if the message processing succeeds

    switch (message)
    {
    case WM_KEYDOWN:
        is_key_down = true;

    case WM_KEYUP:
        success = keyDownKeyUpSelector(*this, transformSystemKey(wParam, lParam), is_key_down);
        break;

    case WM_CHAR:
    {
        success = character(static_cast<wchar_t>(wParam));
        break;
    }

    case WM_SYSKEYDOWN:
        success = systemKeyDown(transformSystemKey(wParam, lParam));
        break;

    case WM_SYSKEYUP:
        success = systemKeyUp(transformSystemKey(wParam, lParam));
        break;

    default:
        __assume(0);
    }

    return static_cast<uint64_t>(success);
}



int64_t MouseButtonListener::process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
    uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5)
{
    uint16_t x = static_cast<uint16_t>(lParam & 0xFFFF);
    uint16_t y = static_cast<uint16_t>(lParam >> 16);
    ControlKeyFlag control_key_flag = transformControlFlag(static_cast<WPARAM>(wParam));

    MouseButton button;
    bool is_button_pressed = false;
    bool is_button_double_clicked = false;
    bool is_horizontal_wheel_moved = false;
    uint16_t x_button_id = 0xFF;

    switch (message)
    {
    case WM_LBUTTONDOWN:
        is_button_pressed = true;

    case WM_LBUTTONUP:
        button = MouseButton::left;
        break;

    case WM_MBUTTONDOWN:
        is_button_pressed = true;
    case WM_MBUTTONUP:
        button = MouseButton::middle;
        break;

    case WM_RBUTTONDOWN:
        is_button_pressed = true;
    case WM_RBUTTONUP:
        button = MouseButton::right;
        break;

    case WM_XBUTTONDOWN:
        is_button_pressed = true;
    case WM_XBUTTONUP:
        button = MouseButton::x;
        x_button_id = static_cast<uint16_t>(wParam >> 16);
        break;

    case WM_LBUTTONDBLCLK:
        is_button_double_clicked = true;
        button = MouseButton::left;
        break;

    case WM_RBUTTONDBLCLK:
        is_button_double_clicked = true;
        button = MouseButton::right;
        break;

    case WM_XBUTTONDBLCLK:
        is_button_double_clicked = true;
        button = MouseButton::x;
        x_button_id = static_cast<uint16_t>(wParam >> 16);
        break;

    case WM_MOUSEHWHEEL:
        is_horizontal_wheel_moved = true;
    case WM_MOUSEWHEEL:
        return static_cast<uint64_t>(wheelMove(static_cast<double>(wParam >> 16) / WHEEL_DELTA, is_horizontal_wheel_moved, control_key_flag, x, y));

    default:
        __assume(0);
    }

    if (is_button_double_clicked)
    {
        return static_cast<uint64_t>(doubleClick(button, x_button_id, control_key_flag, x, y));
    }
    else
    {
        return static_cast<uint64_t>(is_button_pressed
            ? buttonDown(button, x_button_id, control_key_flag, x, y) 
            : buttonUp(button, x_button_id, control_key_flag, x, y));
    }
}



MouseMoveListener::MouseMoveListener():
    m_mouse_tracking_flags{ TME_HOVER | TME_LEAVE }
{

}


int64_t MouseMoveListener::process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
    uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5)
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

    return static_cast<uint64_t>(success);
}



int64_t WindowSizeChangeListener::process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam,
    uint64_t reserved1, uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5)
{
    uint16_t new_width = lParam & 0xFFFF;
    uint16_t new_height = static_cast<uint16_t>(lParam >> 16);

    switch (wParam)
    {
    case SIZE_MAXIMIZED:
        return static_cast<uint64_t>(maximized(new_width, new_height));

    case SIZE_MINIMIZED:
        return static_cast<uint64_t>(minimized());

    case SIZE_RESTORED:
        return static_cast<uint64_t>(size_changed(new_width, new_height));

    default:
        __assume(0);
    }
}




int64_t ClientAreaUpdateListener::process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1,
    uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5)
{
    bool success = false;    // 'true' if the message processing succeeds

    core::math::Rectangle rect{ core::math::Vector2f{0}, 0, 0 };
    HWND hWnd = reinterpret_cast<Window*>(static_cast<pointer_sized_int<sizeof(Window*)>::type>(p_window))->native();
    RECT windows_rect;

    if (GetUpdateRect(hWnd, &windows_rect, FALSE))
    {
        rect.setUpperLeft(core::math::Vector2f{ static_cast<float>(windows_rect.left), static_cast<float>(windows_rect.top) });
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

    return static_cast<uint64_t>(success);
}



int64_t CursorUpdateListener::process_message(uint64_t message, uint64_t p_window, uint64_t wParam, uint64_t lParam, uint64_t reserved1, uint64_t reserved2, uint64_t reserved3, uint64_t reserved4, uint64_t reserved5)
{
    if ((lParam & 0xFFFF) == HTCLIENT) return static_cast<uint64_t>(setCursor());
    return 0;
}
