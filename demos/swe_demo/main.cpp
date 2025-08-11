#include <cstdlib>
#include <iostream>
#include <sstream>
#include <codecvt>

#include "api/runtime.h"
#include "api/osinteraction/windows/window.h"
#include "api/initializer.h"
#include "api/osinteraction/window_handler.h"
//#include "engine/osinteraction/windows/window.h"
//#include "engine/osinteraction/windows/window_listeners.h"

using namespace lexgine::core;
using namespace lexgine::osinteraction;
using namespace lexgine::osinteraction::windows;
//using namespace lexgine::core::dx::dxgi;
//using namespace lexgine::core::dx::d3d12;

namespace
{

lexgine::EngineSettings createEngineSettings()
{
    lexgine::EngineSettings settings{};

    settings.setEngineApi(EngineApi::Direct3D12);
    auto engineApi = settings.getEngineApi();

    settings.setDebugMode(true);
    settings.setEnableProfiling(true);
    settings.setGlobalSettingsJsonFile("global_settings.json");
    settings.setLoggingOutputPath("");
    settings.setLogName("swe_demo.log");

    auto json_file = settings.getGlobalSettingsJsonFile();

    auto ptr = settings.getNative();


    return settings;
}

lexgine::core::SwapChainDescriptor createSwapChainSettings()
{
    lexgine::core::SwapChainDescriptor swap_chain_desc{};
    swap_chain_desc.color_format = lexgine::core::SwapChainColorFormat::r8_g8_b8_a8_unorm;
    swap_chain_desc.back_buffer_count = 2;
    swap_chain_desc.enable_vsync = false;
    swap_chain_desc.windowed = true;
    return swap_chain_desc;
}

}

class MainClass final
{
public:
    static std::shared_ptr<MainClass> create()
    {
        std::shared_ptr<MainClass> rv = std::shared_ptr<MainClass>{ new MainClass{} };
        return rv;
    }

    ~MainClass()
    {

    }

    void update() const
    {
        m_window_handler->update();
    }

    bool shouldClose() const
    {
        return m_window_handler->shouldClose();
    }

private:
    MainClass()
        : m_engine_settings{ createEngineSettings() }
        , m_engine_initializer{ m_engine_settings }
    {
        m_engine_initializer.setCurrentDevice(0);
        auto device_details = m_engine_initializer.getDeviceDetails();
        std::string device_string_description{};
        {
            using codec = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<codec, wchar_t> converter;
            device_string_description = converter.to_bytes(device_details.description);
        }

        std::cout << device_string_description << std::endl;
        m_window_handler = std::make_unique<lexgine::osinteraction::WindowHandler>(m_engine_initializer.createWindowHandler(m_window, createSwapChainSettings(), lexgine::core::SwapChainDepthFormat::depth32));
       //  m_window.setVisibility(true);
    }

private:
    lexgine::EngineSettings m_engine_settings;
    lexgine::Initializer m_engine_initializer;
    lexgine::osinteraction::windows::Window m_window;
    std::unique_ptr<lexgine::osinteraction::WindowHandler> m_window_handler;

};

int main(int argc, char* argv[])
{
    auto link_result = lexgine::api::linkLexgineApi(LEXGINE_DLL_PATH);


    auto main = MainClass::create();

    while (!main->shouldClose())
    {
        main->update();
    }

    return 0;
}