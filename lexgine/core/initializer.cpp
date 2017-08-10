#include "initializer.h"

#include <windows.h>
#include <algorithm>

using namespace lexgine::core;



namespace {

std::string correct_path(std::string const& original_path)
{
    if (original_path.length() &&
        original_path[original_path.length() - 1] != '/' &&
        original_path[original_path.length() - 1] != '\\')
        return original_path + '/';
    else
        return original_path;
}

}



bool Initializer::m_is_environment_initialized{ false };
bool Initializer::m_is_renderer_initialized{ false };
std::ofstream Initializer::m_logging_file_stream{};
std::vector<std::ofstream> Initializer::m_logging_worker_file_streams{};
std::vector<std::ostream*> Initializer::m_logging_worker_generic_streams{};
std::unique_ptr<GlobalSettings> Initializer::m_global_settings{ nullptr };
std::unique_ptr<Globals> Initializer::m_globals{ nullptr };


bool Initializer::initializeEnvironment(
    std::string const& global_lookup_prefix, 
    std::string const& settings_lookup_path,
    std::string const& global_settings_json_file,
    std::string const& logging_output_path,
    std::string const& log_name)
{
    std::string corrected_logging_output_path = correct_path(logging_output_path);
    std::string corrected_global_lookup_prefix = correct_path(global_lookup_prefix);
    std::string corrected_settings_lookup_path = correct_path(settings_lookup_path);

    // Initialize logging
    {

        m_logging_file_stream.open(corrected_logging_output_path + log_name + ".html", std::ios::out);

        DYNAMIC_TIME_ZONE_INFORMATION time_zone_info;
        DWORD time_zone_id = GetDynamicTimeZoneInformation(&time_zone_info);
        bool dts = time_zone_id == 0 || time_zone_id == 2;
        int8_t time_zone_bias = static_cast<uint8_t>((time_zone_info.Bias + time_zone_info.StandardBias) / 60);

        misc::Log::create(m_logging_file_stream, time_zone_bias, dts);
    }


    m_globals.reset(new Globals{});

    MainGlobalsBuilder builder;

    builder.registerMainLog(m_logging_file_stream);

    m_global_settings.reset(new GlobalSettings{ corrected_global_lookup_prefix + corrected_settings_lookup_path + global_settings_json_file });
    
    // Correct shader lookup directories
    {
        std::vector<std::string> new_shader_lookup_directories;
        for (auto& shader_lookup_path : m_global_settings->getShaderLookupDirectories())
        {
            new_shader_lookup_directories.push_back(correct_path(corrected_global_lookup_prefix + shader_lookup_path));
        }
        m_global_settings->clearShaderLookupDirectories();
        for (auto& new_shader_lookup_path : new_shader_lookup_directories)
        {
            m_global_settings->addShaderLookupDirectory(new_shader_lookup_path);
        }
        m_global_settings->addShaderLookupDirectory(corrected_global_lookup_prefix);
    }

    builder.defineGlobalSettings(*m_global_settings);

    // Create logging streams for workers
    {
        uint8_t num_workers = m_global_settings->getNumberOfWorkers();
        m_logging_worker_file_streams.resize(num_workers);
        for (uint8_t i = 0; i < num_workers; ++i)
        {
            m_logging_worker_file_streams[i].open(corrected_logging_output_path + log_name + "_worker" + std::to_string(i) + ".html", std::ios::out);
        }

        m_logging_worker_generic_streams.resize(m_logging_worker_file_streams.size());
        std::transform(m_logging_worker_file_streams.begin(),
            m_logging_worker_file_streams.end(),
            m_logging_worker_generic_streams.begin(),
            [](std::ofstream& e) -> std::ostream* { return &e; });
        builder.registerWorkerThreadLogs(m_logging_worker_generic_streams);
    }

    *m_globals = builder.build();

    return true;
}

void Initializer::shutdownEnvironment()
{
    misc::Log::shutdown();
    m_logging_file_stream.close();
    for (auto& s : m_logging_worker_file_streams) s.close();
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
    return *m_globals;
}
