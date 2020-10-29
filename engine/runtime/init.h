#ifndef LEXGINE_RUNTIME_INIT_H
#define LEXGINE_RUNTIME_INIT_H

#include <functional>

#include <engine/runtime/external_parser_tokens.h>
#include <engine/runtime/window.h>

// TODO: THESE INCLUDES NEED TO BE REPLACED WITH PROPER INTERFACES
#include <engine/core/dx/d3d12/debug_interface.h>
#include "engine/core/dx/d3d12/swap_chain_link.h"
#include <engine/core/dx/dxgi/hw_adapter_enumerator.h>
#include <engine/osinteraction/windows/window_listeners.h>


namespace lexgine::runtime {

// **************************************************** Engine configuration routines ****************************************************

//! Opaque type, which determines configuration of the engine 
struct _EngineConfiguration
{
    void* _ptr;
};

using EngineConfiguration = _EngineConfiguration const*;
using PreferredGPU = core::dx::dxgi::HwAdapterEnumerator::DxgiGpuPreference;
using GpuBasedValidation = core::dx::d3d12::GpuBasedValidationSettings;


//! General engine configuration settings
struct EngineGeneralSettings
{
    bool debug_mode{ false };
    bool enable_profiling{ false };
    GpuBasedValidation gpu_based_validation{};
    PreferredGPU prioritized_gpu{ PreferredGPU::high_performance };
    uint32_t refresh_rate{ 60 };
    bool stereo_back_buffer{ false };
    bool windowed{ true };
    uint32_t back_buffer_count{ 2 };
    bool vsync_on{ false };
    lexgine::core::dx::d3d12::SwapChainDepthBufferFormat depth_buffer_format{ lexgine::core::dx::d3d12::SwapChainDepthBufferFormat::d32float };
};


//! Creates basic engine configuration
LEXGINE_API EngineConfiguration createEngineConfiguration(EngineGeneralSettings const& settings);

//! Destroys engine configuration
LEXGINE_API void destroyEngineConfiguration(EngineConfiguration engine_configuration);


//! Configures engine path look up prefix
LEXGINE_API void configurePathLookUpPrefix(EngineConfiguration engine_configuration, std::string const& prefix_string);

//! Configures engine global settings path
LEXGINE_API void configureGlobalSettingsPath(EngineConfiguration engine_configuration, std::string const& path);

//! Configures logging output path
LEXGINE_API void configureLoggingPath(EngineConfiguration engine_configuration, std::string const& path);



//! Keyboard input event listener
struct LEXGINE_API KeyboardEventListener
{
    using KeyDownCallback = std::function<bool(lexgine::osinteraction::SystemKey)>;
    using KeyUpCallback = std::function<bool(lexgine::osinteraction::SystemKey)>;
    using CharacterCallback = std::function<bool(wchar_t)>;
    using SystemKeyDownCallback = std::function<bool(lexgine::osinteraction::SystemKey)>;
    using SystemKeyUpCallback = std::function<bool(lexgine::osinteraction::SystemKey)>;

    KeyDownCallback onKeyDown;
    KeyUpCallback onKeyUp;
    CharacterCallback onTypeCharacter;
    SystemKeyDownCallback onSystemKeyDown;
    SystemKeyUpCallback onSystemKeyUp;

    KeyboardEventListener();
};


//! Mouse input event listener
struct LEXGINE_API MouseEventListener
{
    using MouseButton = lexgine::osinteraction::windows::MouseButtonListener::MouseButton;
    using ControlKeyFlag = lexgine::osinteraction::windows::ControlKeyFlag;

    using ButtonDownCallback = std::function<bool(MouseButton, uint16_t, ControlKeyFlag const&, uint16_t, uint16_t)>;
    using ButtonUpCallback = std::function<bool(MouseButton, uint16_t, ControlKeyFlag const&, uint16_t, uint16_t)>;
    using DoubleClickCallback = std::function<bool(MouseButton, uint16_t, ControlKeyFlag const&, uint16_t, uint16_t)>;
    using WheelMoveCallback = std::function<bool(double, bool, ControlKeyFlag const&, uint16_t, uint16_t)>;

    using MoveCallback = std::function<bool(uint16_t, uint16_t, ControlKeyFlag const&)>;
    using EnterClientAreaCallback = std::function<bool()>;
    using LeaveClientAreaCallback = std::function<bool()>;


    ButtonDownCallback onButtonDown;
    ButtonUpCallback onButtonUp;
    DoubleClickCallback onDoubleClick;
    WheelMoveCallback onWheelMove;

    MoveCallback onMove;
    EnterClientAreaCallback onEnterClientArea;
    LeaveClientAreaCallback onLeaveClientArea;

    MouseEventListener();
};

// Window event listener
struct LEXGINE_API WindowEventListener
{
    using MinimizedCallback = std::function<bool()>;
    using MaximizedCallback = std::function<bool(uint16_t, uint16_t)>;
    using SizeChangedCallback = std::function<bool(uint16_t, uint16_t)>;

    using PaintCallback = std::function<bool(core::math::Rectangle const&)>;

    using SetCursorCallback = std::function<bool()>;


    MinimizedCallback onMinimize;
    MaximizedCallback onMaximize;
    SizeChangedCallback onSizeChange;

    PaintCallback onPaint;

    SetCursorCallback onSetCursor;

    WindowEventListener();
};



//! Manages main engine entities (rendering output window, currently used device, swap-chain, etc.)
class LEXGINE_API EngineManager
{
public:
    class impl;

public:
    EngineManager(EngineConfiguration engine_configuration);

    void registerKeyboardEventListener(KeyboardEventListener const& eventListener);
    void registerMouseEventListener(MouseEventListener const& eventListener);
    void registerWindowEventListener(WindowEventListener const& eventListener);

    runtime::osinteraction::windows::Window getMainWindow() const;

private:
    std::unique_ptr<impl> m_impl;
};


// ***************************************************************************************************************************************
}


#endif