#include "3rd_party/imgui/imgui.h"

#include "lexgine/osinteraction/windows/window.h"
#include "ui_draw_task.h"

using namespace lexgine::osinteraction::windows;
using namespace lexgine::core::dx::d3d12::tasks;
using namespace lexgine::core::concurrency;

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

bool UIDrawTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    return true;
}

TaskType UIDrawTask::type() const
{
    return concurrency::TaskType::cpu;
}


