#include "initializer.h"

#include <windows.h>

using namespace lexgine::core;



bool Initializer::m_is_environment_initialized{ false };
bool Initializer::m_is_renderer_initialized{ false };
std::ofstream Initializer::m_logging_file_stream{};
std::vector<std::ofstream> Initializer::m_logging_worker_filer_streams{};
Globals Initializer::m_globals{};


bool Initializer::initializeEnvironment(
    std::string const& global_lookup_prefix, 
    std::string const& settings_lookup_path,
    std::string const& logging_output_path,
    std::string const& log_name)
{
    m_logging_file_stream.open(logging_output_path + log_name + ".txt", std::ios::out);

    DYNAMIC_TIME_ZONE_INFORMATION time_zone_info;
    DWORD time_zone_id = GetDynamicTimeZoneInformation(&time_zone_info);
    bool dts = time_zone_id == 0 || time_zone_id == 2;
    int8_t time_zone_bias = static_cast<uint8_t>((time_zone_info.Bias + time_zone_info.StandardBias) / 60);

    misc::Log::create(m_logging_file_stream, time_zone_bias, dts);


    MainGlobalsBuilder builder;
    GlobalSettings global_settings{ global_lookup_prefix + settings_lookup_path };
    if(global_lookup_prefix.length())
    {
        std::vector<std::string> new_shader_lookup_directories;
        for (auto& shader_lookup_path : global_settings.getShaderLookupDirectories())
        {
            new_shader_lookup_directories.push_back(global_lookup_prefix + shader_lookup_path);
        }
        global_settings.clearShaderLookupDirectories();
        for (auto& new_shader_lookup_path : new_shader_lookup_directories)
        {
            global_settings.addShaderLookupDirectory(new_shader_lookup_path);
        }
    }
    builder.defineGlobalSettings(global_settings);

    uint8_t num_workers = global_settings.getNumberOfWorkers();
    m_logging_worker_filer_streams.resize(num_workers);
    for (uint8_t i = 0; i < num_workers; ++i)
    {
        m_logging_worker_filer_streams[i].open(logging_output_path + log_name + "_worker" + std::to_string(i) + ".txt", std::ios::out);
    }

    return true;
}

void Initializer::shutdownEnvironment()
{
    misc::Log::shutdown();
    m_logging_file_stream.close();
    for (auto& s : m_logging_worker_filer_streams) s.close();
}

bool Initializer::isEnvironmentInitialized()
{
    return m_is_environment_initialized;
}

bool Initializer::isRendererInitialized()
{
    return m_is_renderer_initialized;
}

Globals& Initializer::getGlobalParameterObjectPool()
{
    return m_globals;
}
