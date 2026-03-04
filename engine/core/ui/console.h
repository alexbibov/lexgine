#ifndef LEXGINE_CORE_UI_CONSOLE_H
#define LEXGINE_CORE_UI_CONSOLE_H

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "engine/core/concurrency/lexgine_core_concurrency_fwd.h"
#include "engine/core/ui/ui_provider.h"
#include "engine/core/misc/static_vector.h"
#include "engine/core/misc/log.h"
#include "engine/interaction/console_command.h"
#include "engine/osinteraction/windows/window_listeners.h"

struct ImGuiInputTextCallbackData;

namespace lexgine::core::ui {

class Console : public UIProvider, public osinteraction::Listeners<osinteraction::windows::KeyInputListener>
{
public:
	static std::shared_ptr<Console> create(
		Globals const& globals,
		dx::d3d12::BasicRenderingServices const& basic_rendering_services,
		concurrency::TaskGraph const& task_graph
	);
	lexgine::interaction::console::CommandRegistry& consoleCommandRegistry() { return m_command_registry; }

public:  // required by UIProvider
	void constructUI() override;

public: // KeyInputListener	
	bool character(uint64_t char_key_code) override;  //! called when a character key is pressed; The should returns 'true' on success

private:
	Console(
		Globals const& globals,
		dx::d3d12::BasicRenderingServices const& basic_rendering_services,
		concurrency::TaskGraph const& task_graph
	);

	void addLogEntry(spdlog::details::log_msg const& msg);

	static int InputTextCallbackFn(ImGuiInputTextCallbackData* data);

private:
	static constexpr size_t c_logging_buffer_size = 150;

private:
	struct LogEntry
	{
		std::string message;
		misc::LogMessageType message_type;
	};

	struct ConsoleCommandLineState
	{
		char input_buffer[256]{};
		int carret_position{ 0 };
		std::string last_query{};
		bool open_autocomplete_suggestions{ false };
		bool suggestion_accepted{ false };
		std::vector<std::string> autocomplete_suggestions{};
		int selected_suggestion{ 0 };
		bool command_submitted{ false };
	};

private:
	Globals const& m_globals;
	dx::d3d12::BasicRenderingServices const& m_basic_rendering_services;
	lexgine::interaction::console::CommandRegistry m_command_registry;
	misc::StaticVector<LogEntry, c_logging_buffer_size> m_logging_buffer;
	size_t m_logging_buffer_oldest_entry = 0;
	ConsoleCommandLineState m_command_line_state{};
};

}

#endif