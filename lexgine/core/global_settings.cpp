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
    misc::Optional<std::string> source_json = misc::readAsciiTextFromSourceFile(json_settings_source_path);
    if (!source_json.isValid())
    {
        misc::Log::retrieve()->out("WARNING: unable to parse global settings JSON file located at \"" 
            + json_settings_source_path + "\". The system will fall back to default settings",
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
            misc::Log::retrieve()->out("WARNING: unable to get value for \"number_of_workers\" from settings file located at \""
                + json_settings_source_path + "\". The system will fall back to default value \"number_of_workers = 8\"",
                misc::LogMessageType::exclamation);

            m_number_of_workers = 8U;
        }


        // Deferred pipeline compilation routine settings
        {
            if ((p = document.find("deferred_shader_compilation")) != document.end()
                && p->is_boolean())
            {
                m_deferred_shader_compilation = *p;
            }
            else
            {
                misc::Log::retrieve()->out("WARNING: unable to get value for \"deferred_shader_compilation\" from settings file located at \""
                    + json_settings_source_path + "\". The system will fall back to default value \"deferred_shader_compilation = true\"",
                    misc::LogMessageType::exclamation);

                m_deferred_shader_compilation = true;
            }

            if ((p = document.find("deferred_pso_compilation")) != document.end()
                && p->is_boolean())
            {
                m_deferred_pso_compilation = *p;
            }
            else
            {
                misc::Log::retrieve()->out("WARNING: unable to get value for \"deferred_pso_compilation\" from settings file located at \""
                    + json_settings_source_path + "\". The system will fall back to default value \"deferred_pso_compilation = true\"",
                    misc::LogMessageType::exclamation);

                m_deferred_pso_compilation = true;
            }

            if ((p = document.find("deferred_root_signature_compilation")) != document.end()
                && p->is_boolean())
            {
                m_deferred_root_signature_compilation = *p;
            }
            else
            {
                misc::Log::retrieve()->out("WARNING: unable to get value for \"deferred_root_signature_compilation\" from settings file located at \""
                    + json_settings_source_path + "\". The system will fall back to default value \"deferred_root_signature_compilation = true\"",
                    misc::LogMessageType::exclamation);

                m_deferred_root_signature_compilation = true;
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
                        misc::Log::retrieve()->out("WARNING: unable retrieve value from JSON array \"shader_lookup_directories\" in settings file located at \""
                            + json_settings_source_path + "\"; \"shader_lookup_directories\" is expected to be an array of strings but some of its elements appear to have non-string format.",
                            misc::LogMessageType::exclamation);
                    }
                }
            }
            else
            {
                misc::Log::retrieve()->out("WARNING: unable to get value for \"shader_lookup_directories\" from settings file located at \""
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
            misc::Log::retrieve()->out("WARNING: unable to get value for \"cache_path\" from settings file located at \""
                + json_settings_source_path + "\"; \"cache_path\" must have string value. The system will fall back to default cache path setting.", 
                misc::LogMessageType::exclamation);

            m_cache_path = std::string{ PROJECT_CODE_NAME } +"__v." + std::to_string(PROJECT_VERSION_MAJOR) + "."
                + std::to_string(PROJECT_VERSION_MINOR) + "__rev." + std::to_string(PROJECT_VERSION_REVISION)
                + "__" + PROJECT_VERSION_STAGE + "__cache/";
        }

        if ((p = document.find("combined_cache_name")) != document.end()
            && p->is_string())
        {
            m_combined_cache_name = p->get<std::string>();
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to retrieve value for \"combined_cache_name\" from settings file located at \""
                + json_settings_source_path + "\"; \"combined_cache_name\" either has wrong value (must be string) or was not specified in the settings source file.",
                misc::LogMessageType::exclamation);

            m_combined_cache_name = std::string{ PROJECT_CODE_NAME } +"__v." + std::to_string(PROJECT_VERSION_MAJOR) + "."
                + std::to_string(PROJECT_VERSION_MINOR) + "__rev." + std::to_string(PROJECT_VERSION_REVISION)
                + "__" + PROJECT_VERSION_STAGE + ".combined_cache";
        }

        if ((p = document.find("maximal_combined_cache_size")) != document.end()
            && p->is_number_unsigned())
        {
            m_max_combined_cache_size = p->get<uint64_t>();
        }
        else
        {
            misc::Log::retrieve()->out("WARNING: unable to retrieve value for \"maximal_combined_cache_size\" setting from settings file located at \""
                + json_settings_source_path + "\"; the setting either has not been defined or has invalid value (expected unsigned integer). "
                "Default maximal value of 4GBs will be used instead",
                misc::LogMessageType::exclamation);

            m_max_combined_cache_size = 1024ull * 1024 * 1024 * 4;    // defaults to 4Gbs
        }
    }
    catch (...)
    {
        misc::Log::retrieve()->out("WARNING: JSON file located at \"" + json_settings_source_path + "\" has invalid format. The system will fall back to default settings",
            misc::LogMessageType::exclamation);
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
        { "maximal_combined_cache_size", m_max_combined_cache_size }
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

void GlobalSettings::setNumberOfWorkers(uint8_t num_workers)
{
    m_number_of_workers = num_workers;
}

void GlobalSettings::setIsDeferredShaderCompilationOn(bool is_enabled)
{
    m_deferred_shader_compilation = is_enabled;
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
