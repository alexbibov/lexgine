#ifndef LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_PROFILER_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_PROFILER_TASK_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/osinteraction/windows/lexgine_osinteraction_windows_fwd.h"
#include "lexgine/osinteraction/listener.h"
#include "lexgine/osinteraction/windows/window_listeners.h"

namespace lexgine::core::dx::d3d12::tasks::rendering_tasks {

class ProfilerTask : 
    public osinteraction::Listeners<
    osinteraction::windows::KeyInputListener, 
    osinteraction::windows::MouseButtonListener, 
    osinteraction::windows::MouseMoveListener
    >
{
public:
    ProfilerTask(Globals& globals, BasicRenderingServices& basic_rendering_services,
        osinteraction::windows::Window& rendering_window);

public:    // KeyInputListener events
    bool keyDown(osinteraction::SystemKey key) override;
    bool keyUp(osinteraction::SystemKey key) override;
    bool character(wchar_t char_key) override;
    bool systemKeyDown(osinteraction::SystemKey key) override;
    bool systemKeyUp(osinteraction::SystemKey key) override;

public:    // MouseButtonListener events
    bool buttonDown(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;
    bool buttonUp(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;
    bool doubleClick(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;
    bool wheelMove(double move_delta, bool is_horizontal_wheel, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;

public:    // MouseMoveListener events
    bool move(uint16_t x, uint16_t y, osinteraction::windows::ControlKeyFlag const& control_key_flag) override;
    bool enter_client_area() override;
    bool leave_client_area() override;

private:
    Globals& m_globals;
    BasicRenderingServices& m_basic_rendering_services;
    osinteraction::windows::Window& m_rendering_window;
};

}

#endif
