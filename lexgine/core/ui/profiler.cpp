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

}

Profiler::Profiler(Globals const& globals, TaskGraph const& task_graph)
    : m_globals{ globals }
    , m_task_graph{ task_graph }
    , m_show_profiler{ true }
{

}

void Profiler::constructUI()
{
    // Update values used by the UI
    for (auto& node : m_task_graph)
    {
        AbstractTask* p_task = node.task();

        auto p = m_profiling_summaries.find(p_task);
        if (p == m_profiling_summaries.end() && !p_task->profilingServices().empty())
        {
            p = m_profiling_summaries.emplace(std::make_pair(p_task, std::vector<ProfilingSummary>(p_task->profilingServices().size()))).first;

            int c{ 0 };
            for (auto& profiling_service : p_task->profilingServices())
            {
                p->second[c].name = profiling_service->name();
                p->second[c].group_id = profiling_service->uid();
                ++c;
            }
        }

        int c{ 0 };
        for (auto& profiling_service : p_task->profilingServices())
        {
            auto& stats = profiling_service->statistics();
            p->second[c].average_execution_time_ms = std::accumulate(stats.begin(), stats.end(), 0.0,
                [&stats](double c, double n) { return c + n / stats.size(); }) / profiling_service->timingFrequency() * 1e3;
            ++c;
        }
    }


    // Draw UI when required
    ImGui::SetNextWindowPos(ImVec2{ 0.f, 0.f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2{ 320.f, 240.f }, ImGuiCond_Once);
    if (ImGui::Begin("Profiler", &m_show_profiler, ImGuiWindowFlags_AlwaysAutoResize))
    {
        {
            ImVec4 const cpu_legend_color = convertPixColorMarkerToImGuiColor(pix_marker_colors::PixCPUJobMarkerColor);
            ImVec4 const gpu_general_legend_color = convertPixColorMarkerToImGuiColor(pix_marker_colors::PixGPUGeneralJobColor);
            ImVec4 const gpu_graphics_legend_color = convertPixColorMarkerToImGuiColor(pix_marker_colors::PixGPUGraphicsJobMarkerColor);
            ImVec4 const gpu_compute_legend_color = convertPixColorMarkerToImGuiColor(pix_marker_colors::PixGPUComputeJobMarkerColor);
            ImVec4 const gpu_copy_legend_color = convertPixColorMarkerToImGuiColor(pix_marker_colors::PixGPUCopyJobMarkerColor);


            ImGui::BeginGroup();
            ImGui::Text("Legend");
            ImGui::TextColored(cpu_legend_color, "CPU job");
            ImGui::TextColored(gpu_graphics_legend_color, "GPU graphics job");
            ImGui::TextColored(gpu_compute_legend_color, "GPU compute job");
            ImGui::TextColored(gpu_copy_legend_color, "GPU copy job");
            ImGui::TextColored(gpu_general_legend_color, "GPU general job");
            ImGui::EndGroup();
        }

        for (auto& [p_task, summaries] : m_profiling_summaries)
        {
            if (ImGui::TreeNode(p_task->getStringName().c_str()))
            {
                for (ProfilingSummary const& s : summaries)
                {
                    ImVec4 color = convertPixColorMarkerToImGuiColor(s.group_id);
                    ImGui::TextColored(color, (s.name + ":  %fms").c_str(), s.average_execution_time_ms);
                }

                ImGui::TreePop();
            }
        }
    }

    ImGui::End();
}