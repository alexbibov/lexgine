#include "imgui.h"

#include <cstring>

#include "console.h"

namespace lexgine::core::ui {

namespace{

ImVec4 getColorForLogMessage(misc::LogMessageType message_type)
{
	switch (message_type)
	{
	case misc::LogMessageType::debug:
		return { 8.f / 255, 102.f / 255, 22.f / 255, 1.f };
	case misc::LogMessageType::information:
		return { 0.f, 213.f / 255, 1.f, 1.f };
	case misc::LogMessageType::exclamation:
		return { 1.f, 204.f / 255, 0.f, 1.f };
	case misc::LogMessageType::error:
		return { 156.f / 255, 23.f / 255, 67.f / 255, 1.f };
	case misc::LogMessageType::critical:
		return { 1.f, 0.f, 0.f, 1.f };
	case misc::LogMessageType::trace:
	default:
		return { 230.f / 255, 230.f / 255, 230.f / 255, 1.f };
	}
}

int InputTextCaretCallback(ImGuiInputTextCallbackData* data)
{
	if (data && data->UserData)
	{
		*static_cast<int*>(data->UserData) = data->CursorPos;
	}
	return 0;
}

}  // namespace

std::shared_ptr<Console> Console::create(
	Globals const& globals, 
	dx::d3d12::BasicRenderingServices const& basic_rendering_services, 
	concurrency::TaskGraph const& task_graph
)
{
	auto rv = std::shared_ptr<Console>{ new Console{globals, basic_rendering_services, task_graph} };
	std::weak_ptr<Console> console_ptr{ rv };
	misc::Log::retrieve()->registerLogListener(
		[console_ptr]
		(spdlog::details::log_msg const& msg) 
		{
			if (!console_ptr.expired())
			{
				console_ptr.lock()->addLogEntry(msg);
			}
		}
	);

	{
		interaction::console::CommandSpec logging_test_command{
			.name = "Log",
			.summary = "Adds provided line to log"
		};

		auto log_message_type_parser = [console_ptr](std::string_view const& token) -> interaction::console::ParserOutput
			{
				interaction::console::ParserOutput output = interaction::console::value_parsers::int_parser(token);
				if (output.value.has_value())
				{
					int value = std::get<int>(*output.value);
					if (value >= 0 && value <= static_cast<int>(misc::LogMessageType::critical))
					{
						return output;
					}
					else
					{
						return interaction::console::ParserOutput{ .value = std::nullopt, .error_message = "Invalid log messsage type" };
					}
				}
				else
				{
					return output;
				}
			};
		logging_test_command.args.push_back(
			{ 
				.name = "message", 
				.description = "Message to log", 
				.parser = interaction::console::value_parsers::string_parser 
			}
		);
		logging_test_command.args.push_back(
			{
				.name = "type",
				.description = "Type of the message to add to the log. Accepted values are: \n"
				"0 trace\n"
				"1 debug\n"
				"2 information\n"
				"3 exclamation\n"
				"4 error"
				"5 critical",
				.parser = log_message_type_parser
			}
		);

		rv->m_command_registry.addCommand(
			logging_test_command, 
			[](interaction::console::ArgMap const& args) -> interaction::console::CommandExecResult
			{
				std::string message = std::get<std::string>(args.at("message"));
				misc::LogMessageType message_type = static_cast<misc::LogMessageType>(std::get<int>(args.at("type")));
				if (misc::Log* p_log = misc::Log::retrieve())
				{
					p_log->out(message, message_type);
					return interaction::console::CommandExecResult{ .succeeded = true };
				}
				return interaction::console::CommandExecResult{ .succeeded = false, .msg = "Logger is not available" };
			}
		);
	}

	return rv;
}

void Console::constructUI()
{
	// Rigid top attachment: use main viewport, set pos/size every frame, disable decorations/move/resize.
	ImGuiViewport* vp = ImGui::GetMainViewport();
	// ImGui::SetNextWindowViewport(vp->ID);
	ImGui::SetNextWindowPos(vp->Pos);
	ImGui::SetNextWindowSize(ImVec2(vp->Size.x, vp->Size.y * 0.35f));

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus; // keeps it from stealing focus when appearing

	ImGui::SetNextWindowBgAlpha(0.85f);

	if (!ImGui::Begin("Console (rigid top)", nullptr, flags)) { ImGui::End(); return; }

	// Layout: Child for log (scrollable), separator, then input line
	const float footer_h = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y * 2.0f;
	ImGui::BeginChild("ScrollRegion", ImVec2(0, -footer_h), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

	// Log lines
	ImGuiListClipper clipper;
	if (!m_logging_buffer.empty())
	{
		clipper.Begin(m_logging_buffer.size());
		while (clipper.Step()) {
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
				LogEntry& log_entry = m_logging_buffer[(i + m_logging_buffer_oldest_entry) % m_logging_buffer.size()];
				ImGui::PushStyleColor(ImGuiCol_Text, getColorForLogMessage(log_entry.message_type));
				ImGui::TextUnformatted(log_entry.message.c_str());
				ImGui::PopStyleColor();
			}
		}
		clipper.End();
	}
	ImGui::EndChild();
	ImGui::Separator();

	// Input line
	ImGui::PushItemWidth(-1);
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGuiInputTextFlags input_flags =
		ImGuiInputTextFlags_EnterReturnsTrue |
		ImGuiInputTextFlags_CallbackHistory |
		ImGuiInputTextFlags_CallbackCompletion |
		ImGuiInputTextFlags_CallbackAlways;

	// Track rect for popup placement
	ImGui::SetNextItemAllowOverlap();
	bool submit = false;
	static int caret = 0;

	static char console_query_buf[100];
	ImGui::PushID("ConsoleInput");
	if (ImGui::InputText("##input",
		console_query_buf,
		sizeof(console_query_buf),
		input_flags,
		InputTextCaretCallback,
		&caret)) {
		submit = true;
	}

	static std::string last_query{};
	static std::vector<std::string> suggestions{};
	static bool sugg_open = false;
	static int sugg_selected = 0;
	std::string current_query{ console_query_buf };
	if(current_query != last_query)
	{
		m_command_registry.setQuery(current_query);
		std::vector<std::string> new_suggestions = m_command_registry.autocompleteSuggestions(2);
		sugg_open = ImGui::IsItemActive() && !new_suggestions.empty();
		if (new_suggestions != suggestions)
		{
			suggestions = std::move(new_suggestions);
			sugg_selected = 0;
		}
		last_query = current_query;
	}

	auto applySuggestion = [&](std::string const& suggestion)
	{
		std::string updated{ console_query_buf };
		size_t last_sep = updated.find_last_of(" \t\r\n");
		if (last_sep == std::string::npos)
		{
			updated = suggestion;
		}
		else if (last_sep + 1 >= updated.size())
		{
			updated += suggestion;
		}
		else
		{
			updated.erase(last_sep + 1);
			updated += suggestion;
		}
		std::strncpy(console_query_buf, updated.c_str(), sizeof(console_query_buf));
		console_query_buf[sizeof(console_query_buf) - 1] = '\0';
		caret = static_cast<int>(std::strlen(console_query_buf));
	};

	if (sugg_open) {
		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();
		ImVec2 text_start = ImVec2(min.x + ImGui::GetStyle().FramePadding.x, min.y + ImGui::GetStyle().FramePadding.y);
		ImVec2 text_size = ImGui::CalcTextSize(console_query_buf, console_query_buf + caret);
		float caret_x = text_start.x + text_size.x;
		if (caret_x > max.x)
		{
			caret_x = max.x;
		}
		ImGui::SetNextWindowPos(ImVec2(caret_x, max.y));
		ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(FLT_MAX, ImGui::GetTextLineHeightWithSpacing() * 8.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
		ImGui::OpenPopup("CmdSuggest");
		if (ImGui::BeginPopup("CmdSuggest",
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav)) {
			if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { ImGui::CloseCurrentPopup(); sugg_open = false; }
			if (!suggestions.empty()) {
				if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) sugg_selected = (sugg_selected + 1) % static_cast<int>(suggestions.size());
				if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) sugg_selected = (sugg_selected + static_cast<int>(suggestions.size()) - 1) % static_cast<int>(suggestions.size());
			}

			bool accept_now = ImGui::IsKeyPressed(ImGuiKey_Tab);
			for (int i = 0; i < static_cast<int>(suggestions.size()); ++i) {
				const bool sel = (i == sugg_selected);
				if (ImGui::Selectable((suggestions[i] + "##" + std::to_string(i)).c_str(), sel)) {
					sugg_selected = i;
					accept_now = true;
				}
			}

			if (accept_now && !suggestions.empty()) {
				applySuggestion(suggestions[sugg_selected]);
				ImGui::CloseCurrentPopup();
				sugg_open = false;
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
	}
	ImGui::PopID();

	//// Submit on Enter
	//if (submit) {
	//	std::string cmd = ui.input;
	//	if (!cmd.empty()) {
	//		ExecCommand(ui, cmd);
	//		ui.history.erase(std::remove(ui.history.begin(), ui.history.end(), cmd), ui.history.end());
	//		ui.history.push_back(cmd);
	//	}
	//	ui.history_pos = -1;
	//	ui.input[0] = '\0';
	//}

	//// Hotkeys
	//if (ImGui::IsItemActive()) {
	//	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_L, false)) ui.Clear();      // Ctrl+L clear
	//	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_K, false)) ui.Clear();      // Ctrl+K clear
	//}

	//ImGui::PopItemWidth();
	ImGui::End();
}

bool Console::character(uint64_t char_key_code)
{
	if (char_key_code == '`')
	{
		setEnabledState(!isEnabled());
	}

	return true;
}

Console::Console(
	Globals const& globals,
	dx::d3d12::BasicRenderingServices
	const& basic_rendering_services,
	concurrency::TaskGraph const& task_graph
)
	: m_globals{ globals }
	, m_basic_rendering_services{ basic_rendering_services }
{
}

void Console::addLogEntry(spdlog::details::log_msg const& msg)
{
	misc::LogMessageType message_type{};
	switch (msg.level)
	{
	case spdlog::level::trace:
		message_type = misc::LogMessageType::trace;
		break;
	case spdlog::level::debug:
		message_type = misc::LogMessageType::debug;
		break;
	case spdlog::level::info:
		message_type = misc::LogMessageType::information;
		break;
	case spdlog::level::warn:
		message_type = misc::LogMessageType::exclamation;
		break;
	case spdlog::level::err:
		message_type = misc::LogMessageType::error;
		break;
	case spdlog::level::critical:
		message_type = misc::LogMessageType::critical;
		break;
	case spdlog::level::off:
		return;
	}
	if (m_logging_buffer.size() < c_logging_buffer_size)
	{
		
		m_logging_buffer.push_back({ .message = msg.payload.data(), .message_type = message_type});
	}
	else
	{
		m_logging_buffer[m_logging_buffer_oldest_entry] = { .message = msg.payload.data(), .message_type = message_type};
		m_logging_buffer_oldest_entry = (m_logging_buffer_oldest_entry + 1) % c_logging_buffer_size;
	}
}

}