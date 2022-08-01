#ifndef LEXGINE_OSINTERACTION_WINDOW_HANDLER_H
#define LEXGINE_OSINTERACTION_WINDOW_HANDLER_H

#include <engine/preprocessing/preprocessor_tokens.h>
#include <engine/osinteraction/windows/window.h>
#include <engine/osinteraction/keyboard.h>
#include <engine/osinteraction/mouse.h>

namespace lexgine::osinteraction {

class LEXGINE_CPP_API WindowHandler final
{
public:
    enum class LEXGINE_CPP_API KeyInputEvent {
        KEY_DOWN,
        KEY_UP,
        CHARACTER,
        SYSTEM_KEY_DOWN,
        SYSTEM_KEY_UP
    };

    union LEXGINE_CPP_API KeyInputSource
    {
        SystemKey system_key;
        wchar_t character;
    };

    struct LEXGINE_CPP_API KeyInputInfo
    {
        KeyInputSource event_source;
    };


    enum class LEXGINE_CPP_API MouseInputEvent {
        BUTTON_DOWN,
        BUTTON_UP,
        DOUBlE_CLICK,
        WHEEL_MOVE
    };

    union LEXGINE_CPP_API MouseInputSource
    {
        MouseButton button;
        double wheel_move_delta;
    };

    union LEXGINE_CPP_API MouseInputSourceEx
    {
        uint16_t button_event_xbutton_id;
        bool wheel_move_is_horizontal_wheel;
    };

    struct LEXGINE_CPP_API MouseInputInfo
    {
        MouseInputSource event_source;
        MouseInputSourceEx event_source_ex;
        uint16_t x, y;
        ControlKeyFlag control_key;
    };

    

    enum class LEXGINE_CPP_API MouseMoveEvent {
        MOVE,
        ENTERED_CLIENT_AREA,
        LEFT_CLIENT_AREA
    };

    struct LEXGINE_CPP_API MouseMoveInfo
    {
        uint16_t x, y;
        ControlKeyFlag control_key;
    };

public:
    using KeyInputHandler = bool(*)(KeyInputInfo const&, KeyInputEvent);
    using MouseButtonHandler = bool(*)(MouseInputInfo const&, MouseInputEvent);
    using MouseMoveHandler = bool(*)(MouseMoveInfo const&, MouseMoveEvent);

public:
    WindowHandler(windows::Window& os_window);
    ~WindowHandler();

    LEXGINE_CPP_API void enableKeyInputMonitoring(KeyInputHandler key_input_handler);
    LEXGINE_CPP_API void enableMouseButtonMonitoring(MouseButtonHandler mouse_button_handler);
    LEXGINE_CPP_API void enableMouseMoveMonitoring(MouseMoveHandler mouse_move_handler);
    LEXGINE_CPP_API windows::Window* attachedWindow() const;

private:
    class impl;

private:
    windows::Window& m_target_os_window;
    std::unique_ptr<impl> m_impl;
};

}

#endif