#include <cstdlib>
#include <iostream>
#include <sstream>

#include "../lexgine/window.h"
#include "../lexgine/window_listeners.h"
#include "../lexgine/hw_adapter_enumerator.h"
#include "../lexgine/debug_interface.h"

using namespace lexgine::osinteraction;
using namespace lexgine::osinteraction::windows;
using namespace lexgine::core;
using namespace lexgine::core::dx::dxgi;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::misc;

class WindowEventListener : public Listeners<KeyInputListener, MouseButtonListener, MouseMoveListener, WindowSizeChangeListener, ClientAreaUpdateListener>
{
public:
    bool keyDown(SystemKey key) const override
    {
        std::cout << "key with code " << static_cast<uint32_t>(key) << " pressed" << std::endl;
        return true;
    }

    bool keyUp(SystemKey key) const override
    {
        std::cout << "key with code " << static_cast<uint32_t>(key) << " released" << std::endl;
        return true;
    }

    bool character(wchar_t char_key) const override
    {
        std::cout << "character " << char_key << " received" << std::endl;
        return true;
    }



    bool buttonDown(MouseButton button, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) const override
    {
        switch (button)
        {
        case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::left:
            std::cout << "left mouse button pressed" << std::endl;
            break;
        case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::middle:
            std::cout << "middle mouse button pressed" << std::endl;
            break;
        case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::right:
            std::cout << "right mouse button pressed" << std::endl;
            break;
        }
        return true;
    }

    bool buttonUp(MouseButton button, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) const override
    {
        switch (button)
        {
        case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::left:
            std::cout << "left mouse button released" << std::endl;
            break;
        case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::middle:
            std::cout << "middle mouse button released" << std::endl;
            break;
        case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::right:
            std::cout << "right mouse button released" << std::endl;
            break;
        }
        return true;
    }

    bool wheelMove(double move_delta, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) const override
    {
        std::cout << "mouse wheel moved, delta=" << move_delta << std::endl;
        return true;
    }



    bool move(uint16_t x, uint16_t y, ControlKeyFlag const& control_key_flag) const override
    {
        std::cout << "mouse moves at position (" << x << "," << y << ")" << std::endl;
        return true;
    }

    bool enter_client_area() const override
    {
        std::cout << "mouse enters client area" << std::endl;
        return true;
    }

    bool leave_client_area() const override
    {
        std::cout << "mouse leaves client area" << std::endl;
        return true;
    }



    bool minimized() const override
    {
        std::cout << "window has been minimized" << std::endl;
        return true;
    }

    bool maximized(uint16_t new_width, uint16_t new_height) const override
    {
        std::cout << "window has been maximized. The new size is (" << new_width << ", " << new_height << ")" << std::endl;
        return true;
    }

    bool size_changed(uint16_t new_width, uint16_t new_height) const override
    {
        std::cout << "window  size has been changed. The new size is (" << new_width << ", " << new_height << ")" << std::endl;
        return true;
    }

    bool paint(lexgine::core::math::Rectangle const& update_region) const override
    {
        return true;
    }


    private:
        lexgine::core::dx::d3d12::CommandList* p_cmd_list;
};

int main(int argc, char* argv[])
{
    lexgine::core::misc::Log::create(std::cout);
    lexgine::core::dx::d3d12::DebugInterface::retrieve();
    {
        HwAdapter default_adapter = *(HwAdapterEnumerator{}.begin());

        Window window{};

        SwapChainDescriptor desc;
        desc.bufferCount = 2;
        desc.bufferUsage = ResourceUsage{ ResourceUsage::enum_type::render_target };
        desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.refreshRate = 60;
        desc.scaling = SwapChainScaling::stretch;
        desc.stereo = false;
        desc.windowed = true;
        SwapChain sc = default_adapter.createSwapChain(window, desc);

        window.setDimensions(lexgine::core::math::vector2u{ 1280, 720 });
        WindowEventListener main_class;
        window.addListener(&main_class);
        window.setVisibility(true);

        while (!window.shouldClose())
        {
            MSG msg;
            BOOL res = GetMessage(&msg, NULL, NULL, NULL);
            if (res)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else break;
        }
    }

    lexgine::core::dx::d3d12::DebugInterface::shutdown();
    lexgine::core::misc::Log::retrieve()->out("alive entities: " + std::to_string(lexgine::core::Entity::aliveEntities()));
    lexgine::core::misc::Log::shutdown();


    return 0;
}