#include "initializer.h"
#include "exception.h"
#include "lexgine/core/build_info.h"
#include "lexgine/core/misc/misc.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/dx/d3d12/dx_resource_factory.h"

#include "lexgine/core/dx/d3d12/task_caches/hlsl_compilation_task_cache.h"
#include "lexgine/core/dx/d3d12/task_caches/pso_compilation_task_cache.h"
#include "lexgine/core/dx/d3d12/task_caches/root_signature_compilation_task_cache.h"
#include "lexgine/core/dx/d3d12/task_caches/combined_cache_key.h"

#include "lexgine/core/dx/d3d12/tasks/hlsl_compilation_task.h"
#include "lexgine/core/dx/d3d12/tasks/pso_compilation_task.h"
#include "lexgine/core/dx/d3d12/tasks/root_signature_compilation_task.h"

#include <windows.h>
#include <algorithm>
#include <cwchar>

using namespace lexgine;
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



Initializer::Initializer(
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
        if (!m_logging_file_stream)
        {
            throw std::runtime_error{ "unable to initialize main logging stream" };
        }

        DYNAMIC_TIME_ZONE_INFORMATION time_zone_info;
        DWORD time_zone_id = GetDynamicTimeZoneInformation(&time_zone_info);
        bool dts = time_zone_id == 0 || time_zone_id == 2;
        int8_t time_zone_bias = static_cast<uint8_t>(-(time_zone_info.Bias + time_zone_info.StandardBias) / 60);

        wchar_t host_computer_name[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD host_computer_name_length;
        if (!GetComputerName(host_computer_name, &host_computer_name_length))
            wcscpy_s(host_computer_name, L"UNKNOWN_HOST");

        std::string log_name{ std::string{ PROJECT_CODE_NAME } +" v." + std::to_string(PROJECT_VERSION_MAJOR) + "."
        + std::to_string(PROJECT_VERSION_MINOR) + " rev." + std::to_string(PROJECT_VERSION_REVISION)
        + "(" + PROJECT_VERSION_STAGE + ")" + " (" + misc::wstringToAsciiString(host_computer_name) + ")" };

        misc::Log::create(m_logging_file_stream, log_name, time_zone_bias, dts);
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

    // Correct cache path
    {
        std::string corrected_cache_path = correct_path(corrected_global_lookup_prefix + m_global_settings->getCacheDirectory());
        m_global_settings->setCacheDirectory(corrected_cache_path);

        if (!CreateDirectory(misc::asciiStringToWstring(corrected_cache_path).c_str(), NULL) &&
            GetLastError() != ERROR_ALREADY_EXISTS)
        {
            misc::Log::retrieve()->out("Unable to create cache path \"" + corrected_cache_path + "\". Caching may not function properly, which "
                "may deteriorate performance of the system", misc::LogMessageType::exclamation);
        }
    }

    builder.defineGlobalSettings(*m_global_settings);

    // Create logging streams for workers
    {
        uint8_t num_workers = m_global_settings->getNumberOfWorkers();
        m_logging_worker_file_streams.resize(num_workers);
        for (uint8_t i = 0; i < num_workers; ++i)
        {
            m_logging_worker_file_streams[i].open(corrected_logging_output_path + log_name + "_worker" + std::to_string(i) + ".html", std::ios::out);
            if (!m_logging_worker_file_streams[i])
            {
                LEXGINE_THROW_ERROR("ERROR: unable to initialize logging stream for worker thread " + std::to_string(i));
            }
        }

        m_logging_worker_generic_streams.resize(m_logging_worker_file_streams.size());
        std::transform(m_logging_worker_file_streams.begin(),
            m_logging_worker_file_streams.end(),
            m_logging_worker_generic_streams.begin(),
            [](std::ofstream& e) -> std::ostream* { return &e; });
        builder.registerWorkerThreadLogs(m_logging_worker_generic_streams);
    }


    // Initialize resource factory

    m_resource_factory.reset(new dx::d3d12::DxResourceFactory{ *m_global_settings });
    builder.registerDxResourceFactory(*m_resource_factory);

    // Initialize caches
    {
        m_shader_cache.reset(new dx::d3d12::task_caches::HLSLCompilationTaskCache{});
        builder.registerHLSLCompilationTaskCache(*m_shader_cache);

        m_pso_cache.reset(new dx::d3d12::task_caches::PSOCompilationTaskCache{});
        builder.registerPSOCompilationTaskCache(*m_pso_cache);

        m_rs_cache.reset(new dx::d3d12::task_caches::RootSignatureCompilationTaskCache{});
        builder.registerRootSignatureCompilationTaskCache(*m_rs_cache);
    }

    *m_globals = builder.build();
}

Initializer::~Initializer()
{
    m_globals = nullptr;
    m_global_settings = nullptr;
    m_resource_factory = nullptr;
    m_shader_cache = nullptr;
    m_pso_cache = nullptr;
   

    // Logger must be shutdown the last since many objects may still log stuff on destruction
    misc::Log::shutdown();
    m_logging_file_stream.close();
    for (auto& s : m_logging_worker_file_streams) s.close();
}


core::Globals& Initializer::globals()
{
    return *m_globals;
}
