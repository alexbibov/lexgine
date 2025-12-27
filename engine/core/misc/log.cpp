#include "log.h"

#include <cassert>
#include <chrono>
#include <sstream>



namespace lexgine::core::misc
{

class SpdlogHtmlFormatter : public spdlog::formatter
{
public:
    void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override
    {
		std::stringstream sstream{};

		sstream << "<tr bgcolor=\"";
		switch (msg.level)
		{
		case spdlog::level::level_enum::trace:
			sstream << "#e6e6e6\">" << std::endl;
			break;
		case spdlog::level::level_enum::debug:
			sstream << "#086616\">" << std::endl;
			break;
		case spdlog::level::level_enum::info:
			sstream << "#00d5ff\">" << std::endl;
			break;
        case spdlog::level::level_enum::warn:
			sstream << "#ffcc00\">" << std::endl;
			break;
		case spdlog::level::level_enum::err:
			sstream << "#9c1743\">" << std::endl;
			break;
		case spdlog::level::level_enum::critical:
			sstream << "#ff0000\">" << std::endl;
			break;
		case spdlog::level::level_enum::off:
			sstream << "#f2f2f2\">" << std::endl;
			break;
		}

		sstream << "<td align=\"center\">";
		//Generate preamble for the logging entry
		sstream << "/" << DateTime{ msg.time }.toString() << "</td>";

		//Add message to the preamble
		sstream << "<td align=\"right\">";
		if (msg.level == spdlog::level::level_enum::warn
			|| msg.level == spdlog::level::level_enum::err
			|| msg.level == spdlog::level::level_enum::critical)
		{
			sstream << "<b>";
		}

		sstream << msg.payload.data();

		if (msg.level == spdlog::level::level_enum::warn
			|| msg.level == spdlog::level::level_enum::err
			|| msg.level == spdlog::level::level_enum::critical)
		{
			sstream << "</b>";
		}

		sstream << "</td>" << std::endl << "</tr>";

		//Write entry to the log
		dest.append(sstream.str());
    }

    std::unique_ptr<spdlog::formatter> clone() const override
    {
        return std::make_unique<SpdlogHtmlFormatter>();
    }
};

std::unique_ptr<Log> Log::m_ptr = nullptr;

Log const& Log::create(std::filesystem::path const& log_path, std::string const& log_name, LogMessageType log_level)
{
	static std::string month_name[] = { "January", "February",
		"March", "April", "May",
		"June", "July", "August",
		"September", "October", "November",
		"December" };

    if (!m_ptr)
    {
		m_ptr = std::unique_ptr<Log>{ new Log{log_path} };
    }

    std::vector<spdlog::sink_ptr> sinks{};

    { 
        // Configure console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sinks.push_back(console_sink);
    }
	{
        // Configure file sink
		spdlog::file_event_handlers handlers{};
		handlers.after_open = [&log_name](const spdlog::filename_t& filename, std::FILE* file_stream)
			{
				std::string log_header_string = log_name + "(" + DateTime::now().toString() + ")";
				std::stringstream s{};
				s << "<!DOCTYPE html>" << std::endl;
				s << "<html>" << std::endl;
				s << "<head><title>" << log_header_string << "</title></head>" << std::endl;
				s << "<body>" << std::endl;
				s << "<table>" << std::endl;
				s << "<thead><tr>" << std::endl;
				s << "<th colspan=\"2\">" << log_header_string << "</th>" << std::endl;
				s << "</tr></thead>" << std::endl;
				s << "<tbody>" << std::endl;
				std::fputs(
					s.str().c_str(),
					file_stream
				);
				std::fflush(file_stream);
			};
		handlers.before_close = [](const spdlog::filename_t& filename, std::FILE* file_stream)
			{
				std::stringstream s{};
				s << "</tbody>" << std::endl;
				s << "</table>" << std::endl;
				s << "</body>" << std::endl;
				s << "</html" << std::endl;
				std::fputs(
					s.str().c_str(),
					file_stream
				);
				std::fflush(file_stream);
			};
		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path.string(), true, handlers);
		file_sink->set_formatter(std::make_unique<SpdlogHtmlFormatter>());
        sinks.push_back(file_sink);
	}
	{
		// callback sink
		auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
			[](spdlog::details::log_msg const& msg)
			{
				for (auto const& l : m_ptr->m_log_listeners)
				{
					l(msg);
				}
			}
		);
		sinks.push_back(callback_sink);
	}

    m_ptr->m_logger = std::make_shared<spdlog::logger>(log_name, sinks.begin(), sinks.end());
	m_ptr->m_logger->set_level(static_cast<spdlog::level::level_enum>(static_cast<int>(log_level)));
	m_ptr->out("*****************Log started*****************", LogMessageType::information);

	return *m_ptr.get();
}

Log* Log::retrieve()
{
    return m_ptr.get();
}

bool Log::shutdown()
{
    if (!m_ptr) return false; // nothing to shutdown
	m_ptr->out("*****************End of the log*****************\n\n\n", LogMessageType::information);
	m_ptr.reset();
    return true;
}


void Log::out(std::string const& message, LogMessageType message_type) const
{
    switch (message_type)
    {
    case LogMessageType::trace:
        m_logger->trace(message);
        break;
    case LogMessageType::debug:
		m_logger->debug(message);
        break;
	case LogMessageType::information:
		m_logger->info(message);
		break;
	case LogMessageType::exclamation:
		m_logger->warn(message);
		break;
	case LogMessageType::error:
		m_logger->error(message);
		break;
	case LogMessageType::critical:
		m_logger->critical(message);
		break;
    }
}

}

