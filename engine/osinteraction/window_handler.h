#ifndef LEXGINE_OSINTERACTION_WINDOW_HANDLER_H
#define LEXGINE_OSINTERACTION_WINDOW_HANDLER_H

#include <engine/preprocessing/preprocessor_tokens.h>
#include <engine/osinteraction/lexgine_osinteraction_fwd.h>
#include <engine/osinteraction/windows/window.h>
#include <engine/osinteraction/keyboard.h>
#include <engine/osinteraction/mouse.h>
#include <engine/core/dx/d3d12/swap_chain_link.h>

namespace lexgine {

class Initializer;

}

namespace lexgine::osinteraction {

template<typename T> class WindowHandlerAttorney;

class LEXGINE_CPP_API WindowHandler final
{
    friend class WindowHandlerAttorney<Initializer>;
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
        uint64_t character;
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
    WindowHandler(WindowHandler&&);
    ~WindowHandler();

    LEXGINE_CPP_API void enableKeyInputMonitoring(KeyInputHandler key_input_handler);
    LEXGINE_CPP_API void enableMouseButtonMonitoring(MouseButtonHandler mouse_button_handler);
    LEXGINE_CPP_API void enableMouseMoveMonitoring(MouseMoveHandler mouse_move_handler);
    LEXGINE_CPP_API windows::Window* attachedWindow() const;
    LEXGINE_CPP_API void update() const;
    LEXGINE_CPP_API bool shouldClose() const;

private:
    WindowHandler(core::Globals& globals, windows::Window& os_window, core::dx::d3d12::SwapChainLink& swap_chain_link);

private:
    class impl;

private:
    windows::Window& m_target_os_window;
    std::shared_ptr<impl> m_impl;
};

template<> class WindowHandlerAttorney<Initializer>
{
    friend class Initializer;

private:
    static WindowHandler makeWindowHandler(core::Globals& globals, windows::Window& os_window, core::dx::d3d12::SwapChainLink& swap_chain_link)
    {
        return WindowHandler{ globals, os_window, swap_chain_link };
    }
};

}

#endif