#include <numeric>

#include "lexgine/core/profiling_services.h"
#include "lexgine/core/concurrency/task_graph.h"
#include "lexgine/core/concurrency/abstract_task.h"
#include "lexgine/core/dx/d3d12/pix_support.h"
#include "profiler.h"

using namespace lexgine::core;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::ui;
using namespace lexgine::core::dx::d3d12;

namespace {

ImVec4 convertPixColorMarkerToImGuiColor(uint32_t pix_color_marker)
{
    float const a = (pix_color_marker >> 24) / 255.f;
    float const r = ((pix_color_marker >> 16) & 0xFF) / 255.f;
    float const g = ((pix_color_marker >> 8) & 0xFF) / 255.f;
    float const b = (pix_color_marker & 0xFF) / 255.f;

    return ImVec4{ r, g, b, a };
}

void constructPerformanceViewTreeNode(TaskGraphNode const& node)
{
    AbstractTask* p_task = node.task();

    if (!p_task->profilingServices().empty())
    {
        if (ImGui::TreeNode(p_task->getStringName().c_str()))
        {
            for (auto& profiling_service : p_task->profilingServices())
            {
                auto& data = profiling_service->statistics();
                float average_execution_time =
                    std::accumulate(data.begin(), data.end(), 0.f,
                        [](float accumulated, float current) -> float
                        {
                            return accumulated + current;
                        }
                ) / data.size();

                ImVec4 color = convertPixColorMarkerToImGuiColor(profiling_service->colorUID());
                ImGui::TextColored(color, (profiling_service->name() + ":  %fms").c_str(), average_execution_time);
            }

            ImGui::TreePop();
        }
    }
}

}

Profiler::Profiler(Globals const& globals, TaskGraph const& task_graph)
    : m_globals{ globals }
    , m_task_graph{ task_graph }
    , m_show_profiler{ true }
{

}

void Profiler::constructUI()
{
    ImGui::SetNextWindowPos(ImVec2{ 0.f, 0.f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2{ 320.f, 240.f }, ImGuiCond_Once);
    if (!ImGui::Begin("Profiler", &m_show_profiler, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    for (auto& node : m_task_graph)
    {
        constructPerformanceViewTreeNode(node);
    }

    ImGui::End();
    /*
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (m_show_demo_window)
        ImGui::ShowDemoWindow(&m_show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &m_show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &m_show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&m_clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (m_show_another_window)
    {
        ImGui::Begin("Another Window", &m_show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            m_show_another_window = false;
        ImGui::End();
    }
    */
}