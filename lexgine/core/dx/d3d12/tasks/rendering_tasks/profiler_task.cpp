#include "3rd_party/imgui/imgui.h"

#include "profiler_task.h"

using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;


namespace {

bool mouseButtonHandler(osinteraction::windows::MouseButtonListener::MouseButton button, uint16_t xbutton_id, 
    osinteraction::windows::Window const& window, bool acquire_capture)
{
    ImGuiIO& io = ImGui::GetIO();

    switch (button)
    {
    case osinteraction::windows::MouseButtonListener::MouseButton::left:
        io.MouseDown[0] = acquire_capture;
        break;
    case osinteraction::windows::MouseButtonListener::MouseButton::middle:
        io.MouseDown[2] = acquire_capture;
        break;
    case osinteraction::windows::MouseButtonListener::MouseButton::right:
        io.MouseDown[1] = acquire_capture;
        break;
    case osinteraction::windows::MouseButtonListener::MouseButton::x:
        io.MouseDown[xbutton_id + 2] = acquire_capture;
        break;
    default:
        __assume(0);
    }

    if (acquire_capture)
    {
        if (!ImGui::IsAnyMouseDown() && GetCapture() == NULL)
            SetCapture(window.native());
    }
    else
    {
        if (!ImGui::IsAnyMouseDown() && GetCapture() == window.native())
            ReleaseCapture();
    }

    return true;
}

}


ProfilerTask::ProfilerTask(Globals& globals, BasicRenderingServices& basic_rendering_services,
    osinteraction::windows::Window& rendering_window)
    : m_globals{ globals }
    , m_basic_rendering_services{ basic_rendering_services }
    , m_rendering_window{ rendering_window }
{
    m_rendering_window.addListener(this);

    ImGuiIO& io = ImGui::GetIO();

    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName = "lexgine_profiler";
    io.ImeWindowHandle = m_rendering_window.native();

    io.KeyMap[ImGuiKey_Tab] = static_cast<int>(osinteraction::SystemKey::tab);
    io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int>(osinteraction::SystemKey::arrow_left);
    io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(osinteraction::SystemKey::arrow_right);
    io.KeyMap[ImGuiKey_UpArrow] = static_cast<int>(osinteraction::SystemKey::arrow_up);
    io.KeyMap[ImGuiKey_DownArrow] = static_cast<int>(osinteraction::SystemKey::arrow_down);
    io.KeyMap[ImGuiKey_PageUp] = static_cast<int>(osinteraction::SystemKey::page_up);
    io.KeyMap[ImGuiKey_PageDown] = static_cast<int>(osinteraction::SystemKey::page_down);
    io.KeyMap[ImGuiKey_Home] = static_cast<int>(osinteraction::SystemKey::home);
    io.KeyMap[ImGuiKey_End] = static_cast<int>(osinteraction::SystemKey::end);
    io.KeyMap[ImGuiKey_Insert] = static_cast<int>(osinteraction::SystemKey::insert);
    io.KeyMap[ImGuiKey_Delete] = static_cast<int>(osinteraction::SystemKey::_delete);
    io.KeyMap[ImGuiKey_Backspace] = static_cast<int>(osinteraction::SystemKey::backspace);
    io.KeyMap[ImGuiKey_Space] = static_cast<int>(osinteraction::SystemKey::space);
    io.KeyMap[ImGuiKey_Enter] = static_cast<int>(osinteraction::SystemKey::enter);
    io.KeyMap[ImGuiKey_Escape] = static_cast<int>(osinteraction::SystemKey::esc);
    io.KeyMap[ImGuiKey_A] = static_cast<int>(osinteraction::SystemKey::A);
    io.KeyMap[ImGuiKey_C] = static_cast<int>(osinteraction::SystemKey::C);
    io.KeyMap[ImGuiKey_V] = static_cast<int>(osinteraction::SystemKey::V);
    io.KeyMap[ImGuiKey_X] = static_cast<int>(osinteraction::SystemKey::X);
    io.KeyMap[ImGuiKey_Y] = static_cast<int>(osinteraction::SystemKey::Y);
    io.KeyMap[ImGuiKey_Z] = static_cast<int>(osinteraction::SystemKey::Z);
}

bool ProfilerTask::keyDown(osinteraction::SystemKey key)
{
    return true;
}

bool ProfilerTask::keyUp(osinteraction::SystemKey key)
{
    return true;
}

bool ProfilerTask::character(wchar_t char_key)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(char_key);
    return true;
}

bool ProfilerTask::systemKeyDown(osinteraction::SystemKey key)
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[static_cast<size_t>(key)] = 1;
    return true;
}

bool ProfilerTask::systemKeyUp(osinteraction::SystemKey key)
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[static_cast<size_t>(key)] = 0;
    return true;
}


bool ProfilerTask::buttonDown(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    return mouseButtonHandler(button, xbutton_id, m_rendering_window, true);
}

bool ProfilerTask::buttonUp(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    return mouseButtonHandler(button, xbutton_id, m_rendering_window, false);
}

bool ProfilerTask::doubleClick(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    return buttonDown(button, xbutton_id, control_key_flag, x, y);
}

bool ProfilerTask::wheelMove(double move_delta, bool is_horizontal_wheel, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    ImGuiIO& io = ImGui::GetIO();

    if (is_horizontal_wheel)
    {
        io.MouseWheelH += static_cast<float>(move_delta);
    }
    else
    {
        io.MouseWheel += static_cast<float>(move_delta);
    }

    return true;
}


bool ProfilerTask::move(uint16_t x, uint16_t y, osinteraction::windows::ControlKeyFlag const& control_key_flag)
{
    return true;
}

bool ProfilerTask::enter_client_area()
{
    return true;
}

bool ProfilerTask::leave_client_area()
{
    return true;
}
