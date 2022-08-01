#include "window_handler.h"
#include "engine/osinteraction/windows/window_listeners.h"
#include "engine/osinteraction/windows/window.h"

using namespace lexgine::osinteraction;
using namespace lexgine::osinteraction::windows;


class WindowHandler::impl :
    public Listeners<KeyInputListener, MouseButtonListener, MouseMoveListener>
{
public:    // KeyInputListener
    bool keyDown(SystemKey key) override 
    {
        if(!m_key_input_handler) return true;

        KeyInputInfo info{ .event_source = KeyInputSource{.system_key = key} };
        return m_key_input_handler(info, KeyInputEvent::KEY_DOWN);
    }

    bool keyUp(SystemKey key) override
    {
        if (!m_key_input_handler) return true;

        KeyInputInfo info{ .event_source = KeyInputSource{.system_key = key} };
        return m_key_input_handler(info, KeyInputEvent::KEY_UP);
    }

    bool character(wchar_t char_key) override
    {
        if (!m_key_input_handler) return true;

        KeyInputInfo info{ .event_source = KeyInputSource{.character = char_key} };
        return m_key_input_handler(info, KeyInputEvent::CHARACTER);
    }

    bool systemKeyDown(SystemKey key) override
    {
        if (!m_key_input_handler) return true;

        KeyInputInfo info{ .event_source = KeyInputSource{.system_key = key} };
        return m_key_input_handler(info, KeyInputEvent::SYSTEM_KEY_DOWN);
    }

    bool systemKeyUp(SystemKey key) override
    {
        if (!m_key_input_handler) return true;

        KeyInputInfo info{ .event_source = KeyInputSource{.system_key = key} };
        return m_key_input_handler(info, KeyInputEvent::SYSTEM_KEY_UP);
    }


public:    // MouseButtonListener
    bool buttonDown(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override
    {
        return true;
    }
    
    bool buttonUp(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override
    {
        return true;
    }
    
    bool doubleClick(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override
    {
        return true;
    }

    bool wheelMove(double move_delta, bool is_horizontal_wheel, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override
    {
        return true;
    }


public:    // MouseMoveListener
    bool move(uint16_t x, uint16_t y, ControlKeyFlag const& control_key_flag) override
    {
        return true;
    }

    bool leave_client_area() override
    {
        return true;
    }

    bool enter_client_area() override
    {
        return true;
    }

public:
    void setKeyInputCallbackPtr(KeyInputHandler key_input_handler) { m_key_input_handler = key_input_handler; }
    void setMouseButtonCallbackPtr(MouseButtonHandler mouse_button_handler) { m_mouse_button_handler = mouse_button_handler; }
    void setMouseMoveCallbackPtr(MouseMoveHandler mouse_move_handler) { m_mouse_move_handler = mouse_move_handler; }

private:
    KeyInputHandler m_key_input_handler = nullptr;
    MouseButtonHandler m_mouse_button_handler = nullptr;
    MouseMoveHandler m_mouse_move_handler = nullptr;
};



WindowHandler::WindowHandler(windows::Window& os_window)
    : m_target_os_window{ os_window }
    , m_impl{ new impl{} }
{

}

WindowHandler::~WindowHandler() = default;


void WindowHandler::enableKeyInputMonitoring(KeyInputHandler key_input_handler)
{
    m_impl->setKeyInputCallbackPtr(key_input_handler);
}


void WindowHandler::enableMouseButtonMonitoring(MouseButtonHandler mouse_button_handler)
{
    m_impl->setMouseButtonCallbackPtr(mouse_button_handler);
}


void WindowHandler::enableMouseMoveMonitoring(MouseMoveHandler mouse_move_handler)
{
    m_impl->setMouseMoveCallbackPtr(mouse_move_handler);
}

windows::Window* WindowHandler::attachedWindow() const
{
    return &m_target_os_window;
}