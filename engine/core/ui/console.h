#ifndef LEXGINE_CORE_UI_CONSOLE_H
#define LEXGINE_CORE_UI_CONSOLE_H

#include "imgui.h"
#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "engine/core/concurrency/lexgine_core_concurrency_fwd.h"
#include "engine/core/ui/ui_provider.h"

namespace lexgine::core::ui {

class Console : public UIProvider
{
public:
	static std::shared_ptr<Console> create(
		Globals const& globals,
		dx::d3d12::BasicRenderingServices const& basic_rendering_services,
		concurrency::TaskGraph const& task_graph
	);

public:  // required by UIProvider
	void constructUI() override;
	void setEnabledState(bool enabled);

private:
	Console(
		Globals const& globals,
		dx::d3d12::BasicRenderingServices const& basic_rendering_services,
		concurrency::TaskGraph const& task_graph
	);

private:
	Globals const& m_globals;
	dx::d3d12::BasicRenderingServices const& m_basic_rendering_services;

};

}

#endif