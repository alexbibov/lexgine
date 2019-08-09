#ifndef LEXGINE_CORE_UI_PROFILER_H
#define LEXGINE_CORE_UI_PROFILER_H

#include <chrono>
#include <array>

#include "3rd_party/imgui/imgui.h"
#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "lexgine/core/ui/ui_provider.h"


namespace lexgine::core::ui {

class Profiler : public UIProvider
{

public:
    static std::shared_ptr<Profiler> create(Globals const& globals, concurrency::TaskGraph const& task_graph)
    {
        return std::shared_ptr<Profiler>{new Profiler{ globals, task_graph }};
    }

public:    // required by UIProvider
    void constructUI() override;
    void toggleEnableState() { m_show_profiler = !m_show_profiler; }

private:
    Profiler(Globals const& globals, concurrency::TaskGraph const& task_graph);

private:
    Globals const& m_globals;
    concurrency::TaskGraph const& m_task_graph;
    bool m_show_profiler;

};

}

#endif
