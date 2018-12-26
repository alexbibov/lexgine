#ifndef LEXGINE_CORE_LOGGING_STREAMS_H
#define LEXGINE_CORE_LOGGING_STREAMS_H

#include <vector>
#include <fstream>

namespace lexgine::core {

//! Encapsulates logging streams
struct LoggingStreams final
{
    LoggingStreams() = default;
    ~LoggingStreams();

    std::ofstream main_logging_stream;
    std::ofstream rendering_logging_stream;
    std::vector<std::ofstream> worker_logging_streams;
};





}

#endif
