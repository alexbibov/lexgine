#include <cstdlib>
#include <iostream>
#include <sstream>

#include "api/runtime.h"
#include "api/osinteraction/windows/window.h"
#include "api/core/engine_settings.h"
//#include "engine/osinteraction/windows/window.h"
//#include "engine/osinteraction/windows/window_listeners.h"

using namespace lexgine;
using namespace lexgine::api;
using namespace lexgine::osinteraction;
using namespace lexgine::osinteraction::windows;

//using namespace lexgine::core;
//using namespace lexgine::core::dx::dxgi;
//using namespace lexgine::core::dx::d3d12;

namespace
{

lexgine::core::EngineSettings createEngineSettings()
{
    lexgine::core::EngineSettings settings{};

    settings.setDebugMode(true);
    settings.setEnableProfiling(true);
    settings.setAdapterEnumerationPreference(lexgine::core::dx::dxgi::DxgiGpuPreference::high_performance);
    settings.setGlobalLookupPrefix("");
    settings.setSettingsLookupPath("../../settings/");
    settings.setGlobalSettingsJsonFile("global_settings.json");
    settings.setLoggingOutputPath("");
    settings.setLogName("swe_demo.log");

    auto json_file = settings.getGlobalSettingsJsonFile();

    lexgine::core::dx::d3d12::GpuBasedValidationSettings gpu_based_validation_settings{};
    settings.setGpuBasedValidationSettings(gpu_based_validation_settings);

    return settings;
}
//
//SwapChainDescriptor createSwapChainSettings()
//{
//    SwapChainDescriptor swap_chain_desc{};
//    swap_chain_desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
//    swap_chain_desc.refreshRate = 60;
//    swap_chain_desc.scaling = SwapChainScaling::none;
//    swap_chain_desc.stereo = false;
//    swap_chain_desc.windowed = true;
//    swap_chain_desc.back_buffer_count = 3;
//    swap_chain_desc.enable_vsync = false;
//    return swap_chain_desc;
//}

}

class MainClass final /*:
    public Listeners<
    KeyInputListener,
    MouseButtonListener,
    MouseMoveListener,
    WindowSizeChangeListener,
    ClientAreaUpdateListener
    >*/
{
public:
    static std::shared_ptr<MainClass> create()
    {
        std::shared_ptr<MainClass> rv = std::shared_ptr<MainClass>{ new MainClass{} };
        // rv->m_rendering_window.addListener(rv);
        return rv;
    }

    ~MainClass()
    {

    }

    void update() const
    {
        /*m_rendering_window.update();*/
    }

    bool shouldClose() const
    {
        /*return m_rendering_window.shouldClose();*/
        return false;
    }

    void loop() const
    {
        /*m_rendering_window.processMessages();*/
    }

public:
    //bool keyDown(SystemKey key) override
    //{
    //    // std::cout << "key with code " << static_cast<uint32_t>(key) << " pressed" << std::endl;
    //    return true;
    //}

    //bool keyUp(SystemKey key) override
    //{
    //    // std::cout << "key with code " << static_cast<uint32_t>(key) << " released" << std::endl;
    //    return true;
    //}

    //bool character(wchar_t char_key) override
    //{
    //    // std::cout << "character " << char_key << " received" << std::endl;
    //    return true;
    //}

    //bool systemKeyDown(SystemKey key) override
    //{
    //    return true;
    //}

    //bool systemKeyUp(SystemKey key) override
    //{
    //    return true;
    //}


    //bool buttonDown(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override
    //{
    //    switch (button)
    //    {
    //    case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::left:
    //        // std::cout << "left mouse button pressed" << std::endl;
    //        break;
    //    case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::middle:
    //        // std::cout << "middle mouse button pressed" << std::endl;
    //        break;
    //    case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::right:
    //        //std::cout << "right mouse button pressed" << std::endl;
    //        break;
    //    }
    //    return true;
    //}

    //bool buttonUp(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override
    //{
    //    switch (button)
    //    {
    //    case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::left:
    //        // std::cout << "left mouse button released" << std::endl;
    //        break;
    //    case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::middle:
    //        // std::cout << "middle mouse button released" << std::endl;
    //        break;
    //    case lexgine::osinteraction::windows::MouseButtonListener::MouseButton::right:
    //        // std::cout << "right mouse button released" << std::endl;
    //        break;
    //    }
    //    return true;
    //}

    //bool doubleClick(MouseButton button, uint16_t xbutton_id, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override
    //{
    //    return true;
    //}

    //bool wheelMove(double move_delta, bool is_horizontal_wheel, ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override
    //{
    //    // std::cout << "mouse wheel moved, delta=" << move_delta << std::endl;
    //    return true;
    //}



    //bool move(uint16_t x, uint16_t y, ControlKeyFlag const& control_key_flag) override
    //{
    //    // std::cout << "mouse moves at position (" << x << "," << y << ")" << std::endl;
    //    return true;
    //}

    //bool enter_client_area() override
    //{
    //    // std::cout << "mouse enters client area" << std::endl;
    //    return true;
    //}

    //bool leave_client_area() override
    //{
    //    // std::cout << "mouse leaves client area" << std::endl;
    //    return true;
    //}

    //bool minimized() override
    //{
    //    // std::cout << "window has been minimized" << std::endl;
    //    return true;
    //}

    //bool maximized(uint16_t new_width, uint16_t new_height) override
    //{
    //    // std::cout << "window has been maximized. The new size is (" << new_width << ", " << new_height << ")" << std::endl;
    //    return true;
    //}

    //bool size_changed(uint16_t new_width, uint16_t new_height) override
    //{
    //    // std::cout << "window  size has been changed. The new size is (" << new_width << ", " << new_height << ")" << std::endl;
    //    return true;
    //}

    //bool paint(lexgine::core::math::Rectangle const& update_region) override
    //{
    //    m_swap_chain_link->render();
    //    return true;
    //}

private:
    MainClass()
        : m_engine_settings{ createEngineSettings() }
        /*, m_engine_initializer{m_engine_settings}
        , m_swap_chain{ m_engine_initializer.createSwapChainForCurrentDevice(m_rendering_window, createSwapChainSettings()) }
        , m_rendering_tasks{ m_engine_initializer.createRenderingTasks() }
        , m_swap_chain_link{ m_engine_initializer.createSwapChainLink(m_swap_chain, lexgine::core::dx::d3d12::SwapChainDepthBufferFormat::d32float, *m_rendering_tasks) }*/
    {
        /*m_rendering_window.setLocation(10, 10);
        m_rendering_window.setVisibility(true);

        Device& dev_ref = m_engine_initializer.getCurrentDevice();*/
    }

private:
    lexgine::core::EngineSettings m_engine_settings;
    /*Initializer m_engine_initializer;
    Window m_rendering_window;
    SwapChain m_swap_chain;
    std::unique_ptr<RenderingTasks> m_rendering_tasks;
    std::shared_ptr<SwapChainLink> m_swap_chain_link;*/
};

int main(int argc, char* argv[])
{
    auto link_result = lexgine::api::linkLexgineApi(LEXGINE_DLL_PATH);

    auto main = MainClass::create();
    /*while (!main->shouldClose())
    {
        main->loop();
        main->update();
    }*/

    return 0;
}