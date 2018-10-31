#include "global_settings.h"
#include "initializer.h"

#include "lexgine/core/build_info.h"
#include "lexgine/core/misc/misc.h"
#include "lexgine/core/misc/log.h"


#include "3rd_party/json/json.hpp"
#include <fstream>


using namespace lexgine::core;
using json = nlohmann::json;


GlobalSettings::GlobalSettings(std::string const& json_settings_source_path)
{
    // initialize default values for the settings
    {
        m_number_of_workers = 8U;
        m_deferred_shader_compilation = true;
        // m_deferred_pso_compilation = true;
        // m_deferred_root_signature_compilation = true;
        m_cache_path = std::string{ PROJECT_CODE_NAME } +"__v." + std::to_string(PROJECT_VERSION_MAJOR) + "."
            + std::to_string(PROJECT_VERSION_MINOR) + "__rev." + std::to_string(PROJECT_VERSION_REVISION)
            + "__" + PROJECT_VERSION_STAGE + "__cache/";
        m_combined_cache_name = std::string{ PROJECT_CODE_NAME } +"__v." + std::to_string(PROJECT_VERSION_MAJOR) + "."
            + std::to_string(PROJECT_VERSION_MINOR) + "__rev." + std::to_string(PROJECT_VERSION_REVISION)
            + "__" + PROJECT_VERSION_STAGE + ".combined_cache";
        m_max_combined_cache_size = 1024ull * 1024 * 1024 * 4;    // defaults to 4Gbs
        m_descriptor_heaps_capacity = 512U;
        m_upload_heap_capacity = 1024 * 1024 * 256;    // 256MBs by default
        m_enable_async_compute = true;
        m_enable_async_copy = true;
        m_max_frames_in_flight = 6;
        m_max_non_blocking_upload_buffer_allocation_timeout = 1000U;
    }


    misc::Optional<std::string> source_json = misc::readAsciiTextFromSourceFile(json_settings_source_path);
    
    if (!source_json.isValid())
    {
        misc::Log::retrieve()->out("WARNING: unable to parse global settings JSON file located at \"" 
            + json_settings_source_path + "\". The system will fall back to the default settings",
            misc::LogMessageType::exclamation);
        return;
    }


    try {
        json document = json::parse(static_cast<std::string&>(source_json));

        json::iterator p;


        if ((p = document.find("number_of_workers")) != document.end()
            && p->is_number_unsigned())
        {
            m_number_of_workers = *p;
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to get value for \"number_of_workers\" from the settings file located at \""
                + json_settings_source_path + "\". The system will fall back to the default value \"number_of_workers = " + std::to_string(m_number_of_workers) + "\"",
                misc::LogMessageType::exclamation);
        }


        // Deferred pipeline compilation routine settings
        {
            if ((p = document.find("deferred_pso_compilation")) != document.end()
                && p->is_boolean())
            {
                m_deferred_pso_compilation = *p;
            }
            else
            {
                misc::Log::retrieve()->out("WARNING: unable to get value for \"deferred_pso_compilation\" from the settings file located at \""
                    + json_settings_source_path + "\". The system will fall back to the default value \"deferred_pso_compilation = " + std::to_string(m_deferred_pso_compilation) + "\"",
                    misc::LogMessageType::exclamation);
            }

            if ((p = document.find("deferred_shader_compilation")) != document.end()
                && p->is_boolean())
            {
                m_deferred_shader_compilation = *p;
            }
            else
            {
                m_deferred_shader_compilation = m_deferred_pso_compilation;

                misc::Log::retrieve()->out("WARNING: unable to get value for \"deferred_shader_compilation\" from the settings file located at \""
                    + json_settings_source_path + "\". The system will fall back to the default value \"deferred_shader_compilation = " + std::to_string(m_deferred_shader_compilation) + "\"",
                    misc::LogMessageType::exclamation);
            }

            if ((p = document.find("deferred_root_signature_compilation")) != document.end()
                && p->is_boolean())
            {
                m_deferred_root_signature_compilation = *p;
            }
            else
            {
                m_deferred_root_signature_compilation = m_deferred_pso_compilation;

                misc::Log::retrieve()->out("WARNING: unable to get value for \"deferred_root_signature_compilation\" from the settings file located at \""
                    + json_settings_source_path + "\". The system will fall back to the default value \"deferred_root_signature_compilation = " + std::to_string(m_deferred_root_signature_compilation) + "\"",
                    misc::LogMessageType::exclamation);
            }
        }

        
        if ((p = document.find("shader_lookup_directories")) != document.end())
        {
            if (p->is_array())
            {
                for (auto& e : *p)
                {
                    if (e.is_string())
                    {
                        m_shader_lookup_directories.push_back(e);
                    }
                    else
                    {
                        misc::Log::retrieve()->out("WARNING: unable retrieve value from JSON array \"shader_lookup_directories\" in the settings file located at \""
                            + json_settings_source_path + "\"; \"shader_lookup_directories\" is expected to be an array of strings but some of its elements appear to have non-string format.",
                            misc::LogMessageType::exclamation);
                    }
                }
            }
            else
            {
                misc::Log::retrieve()->out("WARNING: unable to get value for \"shader_lookup_directories\" from the settings file located at \""
                    + json_settings_source_path + "\"; \"shader_lookup_directories\" is expected to be an array of strings but turned out to have different format."
                    " The system will search for shaders in the current working directory",
                    misc::LogMessageType::exclamation);
            }
        }
        
        if ((p = document.find("cache_path")) != document.end()
            && p->is_string())
        {
            m_cache_path = p->get<std::string>();
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to get value for \"cache_path\" from the settings file located at \""
                + json_settings_source_path + "\"; \"cache_path\" must have string value. The system will fall back to default cache path setting (\""
                + m_cache_path + "/\")", misc::LogMessageType::exclamation);
        }

        if ((p = document.find("combined_cache_name")) != document.end()
            && p->is_string())
        {
            m_combined_cache_name = p->get<std::string>();
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to retrieve value for \"combined_cache_name\" from the settings file located at \""
                + json_settings_source_path + "\"; \"combined_cache_name\" either has wrong value (must be string) or was not specified in the settings source file. "
                "The system will revert to use of the default value (\"" + m_combined_cache_name + "\")",
                misc::LogMessageType::exclamation);
        }

        if ((p = document.find("maximal_combined_cache_size")) != document.end()
            && p->is_number_unsigned())
        {
            m_max_combined_cache_size = p->get<uint64_t>();
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to retrieve value for \"maximal_combined_cache_size\" from the settings file located at \""
                + json_settings_source_path + "\"; the setting either has not been defined or has invalid value (expected unsigned integer). "
                "The default maximal value of " + std::to_string(m_max_combined_cache_size / 1024 / 1024 / 1024) + "GBs will be used instead",
                misc::LogMessageType::exclamation);
        }

        if ((p = document.find("descriptor_heaps_capacity")) != document.end()
            && p->is_number_unsigned())
        {
            m_descriptor_heaps_capacity = p->get<uint32_t>();
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to retrieve value for \"descriptor_heaps_capacity\" from the settings file located at \""
                + json_settings_source_path + "\"; the setting either has not been defined or has invalid value (expected unsigned integer). "
                "The default value of " + std::to_string(m_descriptor_heaps_capacity) + " total descriptor count will be used.",
                misc::LogMessageType::exclamation);
        }

        if ((p = document.find("upload_heap_capacity")) != document.end()
            && p->is_number_unsigned())
        {
            m_upload_heap_capacity = p->get<uint32_t>();
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to retrieve value for \"upload_heap_capacity\" from the settings file located at \""
                + json_settings_source_path + "\"; the setting either has not been defined or has invalid value (expected unsigned integer). "
                "The upload heap will be having the default capacity of " + std::to_string(m_upload_heap_capacity / 1024 / 1024) + "MB.",
                misc::LogMessageType::exclamation);
        }

        if ((p = document.find("enable_async_compute")) != document.end()
            && p->is_boolean())
        {
            m_enable_async_compute = p->get<bool>();
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to retrieve value for \"enable_async_compute\" from the settings file located at \""
                + json_settings_source_path + "\"; the setting either has not been defined or has invalid value (expected boolean). "
                "The system will revert to the default setting \"enable_async_compute = " + std::to_string(m_enable_async_compute) + "\"",
                misc::LogMessageType::exclamation);
        }

        if ((p = document.find("enable_async_copy")) != document.end()
            && p->is_boolean())
        {
            m_enable_async_copy = p->get<bool>();
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to retrieve value for \"enable_async_copy\" from the settings file located at \""
                + json_settings_source_path + "\"; the setting either has not been defined or has invalid value (expected boolean). "
                "The system will revert to the default setting \"enable_async_copy = " + std::to_string(m_enable_async_copy) + "\"",
                misc::LogMessageType::exclamation);
        }

        if ((p = document.find("max_frames_in_flight")) != document.end()
            && p->is_number_unsigned())
        {
            m_max_frames_in_flight = p->get<uint16_t>();
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to retrieve value for \"max_frames_in_flight\" from the settings file located at \""
                + json_settings_source_path + "\"; the setting either has not been defined or has invalid value (expected short integer). "
                "The system will revert to the default setting \"max_frames_in_flight = " + std::to_string(m_max_frames_in_flight) + "\"",
                misc::LogMessageType::exclamation);
        }

        if ((p = document.find("max_non_blocking_upload_buffer_allocation_timeout")) != document.end()
            && p->is_number_unsigned())
        {
            m_max_non_blocking_upload_buffer_allocation_timeout = p->get<uint32_t>();
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to retrieve value for \"max_non_blocking_upload_buffer_allocation_timeout\" from the settings file located at \""
                + json_settings_source_path + "\"; the setting either has not been defined or has invalid value (expected unsigned integer). "
                "The system will revert to the default setting \"max_non_blocking_upload_buffer_allocation_timeout = " 
                + std::to_string(m_max_non_blocking_upload_buffer_allocation_timeout) + "\"",
                misc::LogMessageType::exclamation);
        }
    }
    catch (...)
    {
        misc::Log::retrieve()->out("WARNING: JSON file located at \"" + json_settings_source_path + "\" has invalid format. The system will fall back to default settings",
            misc::LogMessageType::exclamation);
    }



    if (!m_deferred_pso_compilation)
    {
        // if deferred pso compilation is disabled then deferred shader and deferred root signature 
        // compilation tasks should also be compiled in immediate mode

        if (m_deferred_shader_compilation)
        {
            misc::Log::retrieve()->out("WARNING: deferred PSO compilation is disabled but deferred shader compilation that PSO compilation relies upon is switched on. "
                "Deferred shader compilation will therefore be forced disabled", misc::LogMessageType::exclamation);
            m_deferred_shader_compilation = false;
        }

        if (m_deferred_root_signature_compilation)
        {
            misc::Log::retrieve()->out("WARNING: deferred PSO compilation is disabled but deferred root signature compilation that PSO compilation relies upon is switched on. "
                "Deferred root signature compilation will therefore be forced disabled", misc::LogMessageType::exclamation);
            m_deferred_root_signature_compilation = false;
        }
    }
}

void GlobalSettings::serialize(std::string const& json_serialization_path) const
{
    std::ofstream ofile{ json_serialization_path, std::ios::out };
    if (!ofile)
    {
        misc::Log::retrieve()->out("WARNING: unable to establish output stream for the destination at \"" + json_serialization_path + "\". The current settings will not be serialized",
            misc::LogMessageType::exclamation);
        return;
    }

    json j = {
        { "number_of_workers", m_number_of_workers},
        { "deferred_shader_compilation", m_deferred_shader_compilation},
        { "deferred_pso_compilation", m_deferred_pso_compilation },
        { "deferred_root_signature_compilation", m_deferred_root_signature_compilation },
        { "cache_path", m_cache_path },
        { "combined_cache_name", m_combined_cache_name },
        { "maximal_combined_cache_size", m_max_combined_cache_size },
        { "descriptor_heaps_capacity", m_descriptor_heaps_capacity },
        { "upload_heap_capacity", m_upload_heap_capacity },
        { "enable_async_compute", m_enable_async_compute },
        { "enable_async_copy", m_enable_async_copy },
        { "max_frames_in_flight", m_max_frames_in_flight },
        { "max_non_blocking_upload_buffer_allocation_timeout", m_max_non_blocking_upload_buffer_allocation_timeout }
    };
    if (m_shader_lookup_directories.size())
        j["shader_lookup_directories"] = m_shader_lookup_directories;

    ofile << j;

    ofile.close();
}

uint8_t GlobalSettings::getNumberOfWorkers() const
{
    return m_number_of_workers;
}

bool GlobalSettings::isDeferredShaderCompilationOn() const
{
    return m_deferred_shader_compilation;
}

bool GlobalSettings::isDeferredPSOCompilationOn() const
{
    return m_deferred_pso_compilation;
}

bool GlobalSettings::isDeferredRootSignatureCompilationOn() const
{
    return m_deferred_root_signature_compilation;
}

std::vector<std::string> const& GlobalSettings::getShaderLookupDirectories() const
{
    return m_shader_lookup_directories;
}

std::string GlobalSettings::getCacheDirectory() const
{
    return m_cache_path;
}

std::string GlobalSettings::getCombinedCacheName() const
{
    return m_combined_cache_name;
}

uint64_t GlobalSettings::getMaxCombinedCacheSize() const
{
    return m_max_combined_cache_size;
}

uint32_t GlobalSettings::getDescriptorPageCapacity(uint32_t page_id) const
{
    auto res = std::div(static_cast<int>(m_descriptor_heaps_capacity), 
        static_cast<int>(m_max_descriptors_per_page));

    if (page_id < static_cast<uint32_t>(res.quot)) return m_max_descriptors_per_page;
    else return static_cast<uint32_t>(res.rem);
}

uint32_t GlobalSettings::getDescriptorPageCount() const
{
    auto res = std::div(static_cast<int>(m_descriptor_heaps_capacity),
        static_cast<int>(m_max_descriptors_per_page));

    return static_cast<uint32_t>(res.quot) + (res.rem != 0 ? 1 : 0);
}

uint32_t GlobalSettings::getUploadHeapCapacity() const
{
    return m_upload_heap_capacity;
}

bool GlobalSettings::isAsyncComputeEnabled() const
{
    return m_enable_async_compute;
}

bool GlobalSettings::isAsyncCopyEnabled() const
{
    return m_enable_async_copy;
}

uint16_t GlobalSettings::getMaxFramesInFlight() const
{
    return m_max_frames_in_flight;
}

uint32_t GlobalSettings::getMaxNonBlockingUploadBufferAllocationTimeout() const
{
    return m_max_non_blocking_upload_buffer_allocation_timeout;
}

void GlobalSettings::setNumberOfWorkers(uint8_t num_workers)
{
    m_number_of_workers = num_workers;
}

void GlobalSettings::setIsDeferredShaderCompilationOn(bool is_enabled)
{
    if (is_enabled && !m_deferred_pso_compilation)
    {
        misc::Log::retrieve()->out("WARNING: cannot enable deferred shader compilation while "
            "deferred PSO compilation is disabled", misc::LogMessageType::exclamation);
        m_deferred_shader_compilation = false;
    }
    else 
        m_deferred_shader_compilation = is_enabled;
}

void GlobalSettings::setIsDeferredPSOCompilationOn(bool is_enabled)
{
    m_deferred_pso_compilation = is_enabled;
}

void GlobalSettings::setIsDeferredRootSignatureCompilationOn(bool is_enabled)
{
    if (is_enabled && !m_deferred_pso_compilation)
    {
        misc::Log::retrieve()->out("WARNING: cannot enable deferred root signature compilation while "
            "deferred PSO compilation is disabled", misc::LogMessageType::exclamation);
        m_deferred_root_signature_compilation = false;
    }
    m_deferred_root_signature_compilation = is_enabled;
}

void GlobalSettings::addShaderLookupDirectory(std::string const & path)
{
    m_shader_lookup_directories.push_back(path);
}

void GlobalSettings::clearShaderLookupDirectories()
{
    m_shader_lookup_directories.clear();
}

void GlobalSettings::setCacheDirectory(std::string const& path)
{
    m_cache_path = path;
}

void GlobalSettings::setCacheName(std::string const& name)
{
    m_combined_cache_name = name;
}

void GlobalSettings::setIsAsyncComputeEnabled(bool is_enabled)
{
    m_enable_async_compute = is_enabled;
}

void GlobalSettings::setIsAsyncCopyEnabled(bool is_enabled)
{
    m_enable_async_copy = is_enabled;
}
