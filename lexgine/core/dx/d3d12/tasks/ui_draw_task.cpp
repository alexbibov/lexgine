#include "3rd_party/imgui/imgui.h"

#include "lexgine/osinteraction/windows/window.h"
#include "ui_draw_task.h"

using namespace lexgine::osinteraction::windows;
using namespace lexgine::core::dx::d3d12::tasks;

namespace {

void initializeImGui(Window const& window)
{

}

}

UIDrawTask::UIDrawTask(Window const& window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& imgui_io = ImGui::GetIO();

    ImGui::StyleColorsDark();
    
    
}


