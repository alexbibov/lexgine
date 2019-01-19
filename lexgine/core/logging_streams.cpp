#include <algorithm>

#include "logging_streams.h"

using namespace lexgine::core;

LoggingStreams::~LoggingStreams()
{
    if (main_logging_stream) main_logging_stream.close();

    std::for_each(worker_logging_streams.begin(), worker_logging_streams.end(),
        [](std::ofstream& s) { if (s) s.close(); });
}