#include "log.h"

#include <chrono>
#include <sstream>


using namespace lexgine::core::misc;



namespace
{
static thread_local Log const* p_thread_logger = nullptr;
}


Log const& Log::create(std::ostream& output_logging_stream, int8_t time_zone /* = 0 */, bool is_dts /* = false */)
{
    if (!p_thread_logger)
    {
        p_thread_logger = new Log(output_logging_stream, time_zone, is_dts);
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
    return p_thread_logger;
}

bool Log::shutdown()
{
    if (!p_thread_logger) return false; // nothing to shutdown
    p_thread_logger->out("*****************End of the log*****************\n\n\n", LogMessageType::information);
    delete p_thread_logger;
    p_thread_logger = nullptr;
    return true;
}


Log::Log(std::ostream& output_logging_stream, int8_t time_zone, bool is_dts) :
    m_time_stamp{ time_zone, is_dts }, m_out_streams{}, m_tabs{ 0 }
{
    m_out_streams.push_back(&output_logging_stream);
}


void Log::out(std::string const& message, LogMessageType message_type) const
{
    std::stringstream sstream{};

    //Generate preamble for the logging entry
    for (int i = 0; i < m_tabs - 1; ++i) sstream << "\t";
    std::string month_name[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
    m_time_stamp = DateTime::now(m_time_stamp.getTimeZone(), m_time_stamp.isDTS());
    sstream << "/" << static_cast<int>(m_time_stamp.day()) << " "
        << month_name[m_time_stamp.month() - 1] << " "
        << static_cast<int>(m_time_stamp.year())
        << "/ (" << static_cast<int>(m_time_stamp.hour())
        << ":" << static_cast<int>(m_time_stamp.minute())
        << ":" << std::to_string(m_time_stamp.second()) << "): ";

    //Add message to the preamble
    sstream << message << std::endl;

    //Write entry to the log
    for (auto& out_stream : m_out_streams)
    {
        *out_stream << sstream.rdbuf()->str();
    }
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
