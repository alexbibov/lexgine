#include "log.h"

#include <cassert>
#include <chrono>
#include <sstream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/logger.h>

namespace lexgine::core::misc
{

class SpdlogHtmlFormatter : public spdlog::formatter
{
public:
    void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override
    {

    }

    std::unique_ptr<spdlog::formatter> clone() const override
    {
        return std::make_unique<SpdlogHtmlFormatter>();
    }
};

Log const& Log::create(std::filesystem::path& log_path, std::string const& log_name)
{
	static std::string month_name[] = { "January", "February",
		"March", "April", "May",
		"June", "July", "August",
		"September", "October", "November",
		"December" };

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    spdlog::file_event_handlers handlers{};
    handlers.after_open = [&log_name](const spdlog::filename_t& filename, std::FILE* file_stream)
        {
            std::stringstream s{};
            std::chrono::zoned_time ztime{ std::chrono::current_zone(), std::chrono::system_clock::now() };
            auto local_time = ztime.get_local_time();
            std::chrono::year_month_day ymd{ std::chrono::floor<std::chrono::days>(local_time) };
            auto day = static_cast<unsigned int>(ymd.day());
            std::string& month = month_name[static_cast<unsigned int>(ymd.month())];

            
            // std::string log_header_string = log_name + std::to_string()

			s << "<!DOCTYPE html>" << std::endl;
			s << "<html>" << std::endl;
			s << "<head><title>" << log_header_string << "</title></head>" << std::endl;
			s << "<body>" << std::endl;
			s << "<table>" << std::endl;
			s << "<thead><tr>" << std::endl;
			s << "<th colspan=\"2\">" << log_header_string << "</th>" << std::endl;
			s << "</tr></thead>" << std::endl;
			s << "<tbody>" << std::endl;
        };


    auto log_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path.string(), false, handlers);


    if (!p_thread_logger)
    {
        p_thread_logger = new Log(output_logging_stream, time_zone, is_dts);

        std::string log_header_string{ log_name + " log output(" +
            DateTime::now(time_zone, is_dts).toString(DateTime::DateOutputMask::year |
                DateTime::DateOutputMask::month |
                DateTime::DateOutputMask::day) + ")" };

        output_logging_stream << "<!DOCTYPE html>" << std::endl;
        output_logging_stream << "<html>" << std::endl;
        output_logging_stream << "<head><title>" << log_header_string << "</title></head>" << std::endl;
        output_logging_stream << "<body>" << std::endl;
        output_logging_stream << "<table>" << std::endl;
        output_logging_stream << "<thead><tr>" << std::endl;
        output_logging_stream << "<th colspan=\"2\">" << log_header_string << "</th>" << std::endl;
        output_logging_stream << "</tr></thead>" << std::endl;
        output_logging_stream << "<tbody>" << std::endl;

        p_thread_logger->out("*****************Log started*****************", LogMessageType::information);
        return *p_thread_logger;
    }
    else
    {
        const_cast<Log*>(p_thread_logger)->m_out_streams.push_back(&output_logging_stream);
        return *p_thread_logger;
    }
}

Log const* Log::retrieve()
{
    if (!p_thread_logger && m_main_logging_stream)
        p_thread_logger = m_main_logging_stream;
    return p_thread_logger;
}

bool Log::shutdown()
{
    if (!p_thread_logger) return false; // nothing to shutdown
    p_thread_logger->out("*****************End of the log*****************\n\n\n", LogMessageType::information);

    {
        std::stringstream sstream{};
        sstream << "</tbody>" << std::endl;
        sstream << "</table>" << std::endl;
        sstream << "</body>" << std::endl;
        sstream << "</html" << std::endl;

        // write terminating tags to every stream attached to the logger
        for (auto& out_stream : p_thread_logger->m_out_streams)
        {
            *out_stream << sstream.rdbuf()->str();
        }
    }

    if(p_thread_logger != m_main_logging_stream)
        delete p_thread_logger;
    p_thread_logger = nullptr;
    m_main_logging_stream = nullptr;
    return true;
}

void Log::registerMainLogger(Log const* logger)
{
    assert(m_main_logging_stream == nullptr || m_main_logging_stream == logger || logger == nullptr);
    m_main_logging_stream = logger;
}


Log::Log(std::ostream& output_logging_stream, int8_t time_zone, bool is_dts) :
    m_time_stamp{ time_zone, is_dts }, m_out_streams{}, m_tabs{ 0 }
{
    m_out_streams.push_back(&output_logging_stream);
}


void Log::out(std::string const& message, LogMessageType message_type) const
{
    bool const is_main_thread = m_main_logging_stream && p_thread_logger == m_main_logging_stream;
    if (is_main_thread)
        m_main_thread_log_mutex.lock();

    std::stringstream sstream{};

    sstream << "<tr bgcolor=\"";
    switch (message_type)
    {
    case LogMessageType::information:
        sstream << "#f2f2f2\">" << std::endl;
        break;
    case LogMessageType::exclamation:
        sstream << "#ffcc00\">" << std::endl;
        break;
    case LogMessageType::error:
        sstream << "#ff0000\">" << std::endl;
        break;
    }

    sstream << "<td align=\"center\">";
    //Generate preamble for the logging entry
    std::string month_name[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
    m_time_stamp = DateTime::now(m_time_stamp.getTimeZone(), m_time_stamp.isDTS());
    sstream << "/" << m_time_stamp.toString() << "</td>";

    //Add message to the preamble
    sstream << "<td align=\"right\">";
    if (message_type == LogMessageType::error || message_type == LogMessageType::exclamation) sstream << "<b>";
    for (int i = 0; i < m_tabs - 1; ++i) sstream << "\t";
    sstream << message;
    if (message_type == LogMessageType::error || message_type == LogMessageType::exclamation) sstream << "</b>";
    sstream << "</td>" << std::endl << "</tr>";

    //Write entry to the log
    for (auto& out_stream : m_out_streams)
    {
        *out_stream << sstream.rdbuf()->str();
    }

    if (is_main_thread)
        m_main_thread_log_mutex.unlock();
}


DateTime const& Log::getLastEntryTimeStamp() const { return m_time_stamp; }



Log::scoped_tabulation_helper::scoped_tabulation_helper(Log& logger) : m_logger{ logger }
{
    ++m_logger.m_tabs;
}

Log::scoped_tabulation_helper::~scoped_tabulation_helper()
{
    if (m_logger.m_tabs > 0) --m_logger.m_tabs;
}

}