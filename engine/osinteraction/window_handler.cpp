#include <vector>

#include "window_handler.h"
#include "engine/osinteraction/windows/window_listeners.h"
#include "engine/osinteraction/windows/window.h"


using namespace lexgine;
using namespace lexgine::osinteraction;
using namespace lexgine::osinteraction::windows;


class WindowHandler::impl :
    public Listeners<KeyInputListener, MouseButtonListener, MouseMoveListener, ClientAreaUpdateListener>
{
public:    // KeyInputListener
    bool keyDown(SystemKey key) override
    {
        if (!m_key_input_handler) return true;

        KeyInputInfo info{ .event_source = KeyInputSource{.system_key = key} };
        return m_key_input_handler(info, KeyInputEvent::KEY_DOWN);
    }

    bool keyUp(SystemKey key) override
    {
        if (!m_key_input_handler) return true;

        KeyInputInfo info{ .event_source = KeyInputSource{.system_key = key} };
        return m_key_input_handler(info, KeyInputEvent::KEY_UP);
    }

    bool character(uint64_t char_key) override
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

public:    // ClientAreaUpdateListener

    bool paint(core::math::Rectangle const& update_region) override
    {
        // the frame is driven by the engine tick; the presented back buffer is what the client area shows
        return true;
    }


public:    // Construction
    impl(core::Globals& globals)
    {

    }


public:
    void setKeyInputCallbackPtr(KeyInputHandler key_input_handler) { m_key_input_handler = key_input_handler; }
    void setMouseButtonCallbackPtr(MouseButtonHandler mouse_button_handler) { m_mouse_button_handler = mouse_button_handler; }
    void setMouseMoveCallbackPtr(MouseMoveHandler mouse_move_handler) { m_mouse_move_handler = mouse_move_handler; }

    void addWaitableHandler(HANDLE handle, std::function<void()> handler)
    {
        m_waitable_handles.push_back(handle);
        m_waitable_handlers.push_back(std::move(handler));
    }

    std::vector<HANDLE> const& waitableHandles() const { return m_waitable_handles; }
    std::function<void()> const& waitableHandler(size_t index) const { return m_waitable_handlers[index]; }

private:
    KeyInputHandler m_key_input_handler = nullptr;
    MouseButtonHandler m_mouse_button_handler = nullptr;
    MouseMoveHandler m_mouse_move_handler = nullptr;

    std::vector<HANDLE> m_waitable_handles;
    std::vector<std::function<void()>> m_waitable_handlers;
};



WindowHandler::WindowHandler(core::Globals& globals, windows::Window& os_window)
    : m_target_os_window{ os_window }
    , m_impl{ new impl{ globals } }
{
    m_target_os_window.addListener(m_impl);
}

WindowHandler::WindowHandler(WindowHandler&&) = default;
WindowHandler::~WindowHandler() = default;

void WindowHandler::addWaitableHandler(HANDLE handle, std::function<void()> handler)
{
    m_impl->addWaitableHandler(handle, std::move(handler));
}


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


void WindowHandler::update() const
{
    // the frames are driven by the rendering thread, so there is nothing to do here until the next message
    // arrives or a waitable handle is signalled. The wait returns at once in either case and only bounds how
    // often the close request is noticed while the window is idle
    std::vector<HANDLE> const& waitable_handles = m_impl->waitableHandles();
    DWORD const handle_count = static_cast<DWORD>(waitable_handles.size());
    DWORD const wait_result = MsgWaitForMultipleObjects(handle_count, waitable_handles.data(), FALSE, 1, QS_ALLINPUT);

    m_target_os_window.processMessages();

    if (wait_result >= WAIT_OBJECT_0 && wait_result < WAIT_OBJECT_0 + handle_count)
    {
        m_impl->waitableHandler(wait_result - WAIT_OBJECT_0)();
    }
}


bool WindowHandler::shouldClose() const
{
    return m_target_os_window.shouldClose();
}

