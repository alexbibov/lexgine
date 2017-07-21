#include "global_settings.h"
#include "../../3rd_party/rapidjson/document.h"
#include "../../3rd_party/rapidjson/ostreamwrapper.h"
#include "../../3rd_party/rapidjson/writer.h"
#include "misc/misc.h"
#include "misc/log.h"
#include "initializer.h"

#include <fstream>

using namespace lexgine::core;
using namespace rapidjson;



GlobalSettings::GlobalSettings(std::string const& json_settings_source_path)
{
    Document document;

    misc::Optional<std::string> source_json = misc::readAsciiTextFromSourceFile(json_settings_source_path);
    if (!source_json.isValid())
    {
        misc::Log::retrieve()->out("WARNING: unable to parse global settings json file located at \"" + json_settings_source_path + "\". The system will fall back to default settings");
        return;
    }

    document.Parse(static_cast<std::string>(source_json).c_str());
    if (!document.IsObject())
    {
        misc::Log::retrieve()->out("WARNING: json file located at \"" + json_settings_source_path + "\" had invalid format. The system will fall back to default settings");
        return;
    }

    
    // number_of_workers
    if (!document["number_of_workers"].IsUint())
    {
        misc::Log::retrieve()->out("WARNING: unable to get value for \"number_of_workers\" from settings file located at \"" 
            + json_settings_source_path + "\". The system will fall back to default value \"number_of_workers = 8\"");
        return;
    }
    else
    {
        m_number_of_workers = static_cast<uint8_t>(document["number_of_workers"].GetUint());
    }

    // deferred_shader_compilation
    if (!document["deferred_shader_compilation"].IsBool())
    {
        misc::Log::retrieve()->out("WARNING: unable to get value for \"deferred_shader_compilation\" from settings file located at \""
            + json_settings_source_path + "\". The system will fall back to default value \"deferred_shader_compilation = true\"");
        return;
    }
    else
    {
        m_deferred_shader_compilation = document["deferred_shader_compilation"].GetBool();
    }

    if (!document["shader_lookup_directories"].IsNull())
    {
        if (!document["shader_lookup_directories"].IsArray())
        {
            misc::Log::retrieve()->out("WARNING: unable to get value for \"shader_lookup_directories\" from settings file located at \""
                + json_settings_source_path + "\"shader_lookup_directories\" is expected to be an array of strings but turned out to have different format."
                " The system will search for shaders in the current working directory");
            return;
        }

        for (auto& e : document["shader_lookup_directory"].GetArray())
        {
            if (!e.IsString())
            {
                misc::Log::retrieve()->out("WARNING: unable to get value for \"shader_lookup_directories\" from settings file located at \""
                    + json_settings_source_path + "\"shader_lookup_directories\" is expected to be an array of strings but some of its elements look like they have a non-string format.");
                return;
            }

            m_shader_lookup_directories.push_back(e.GetString());
        }
    }

}

void GlobalSettings::serialize(std::string const& json_serialization_path) const
{
    std::ofstream ofile{ json_serialization_path, std::ios::out };
    if (!ofile)
    {
        misc::Log::retrieve()->out("WARNING: unable to establish output stream for the destination at \"" + json_serialization_path + "\". The current settings will not be serialized");
        return;
    }

    OStreamWrapper ofile_wrapper{ ofile };
    Writer<OStreamWrapper> w{ ofile_wrapper };
    w.StartObject();
    w.Key("number_of_workers");
    w.Uint(m_number_of_workers);
    w.Key("deferred_shader_compilation");
    w.Bool(m_deferred_shader_compilation);
    if (m_shader_lookup_directories.size())
    {
        w.Key("shader_lookup_directories");
        w.StartArray();
        for (auto& e : m_shader_lookup_directories)
        {
            w.String(e.c_str(), static_cast<SizeType>(e.length()), true);
        }
        w.EndArray();
    }
    w.EndObject();

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

std::vector<std::string> const& GlobalSettings::getShaderLookupDirectories() const
{
    return m_shader_lookup_directories;
}

bool lexgine::core::GlobalSettings::setNumberOfWorkers(uint8_t num_workers)
{
    if (Initializer::isRendererInitialized())
        return false;

    m_number_of_workers = num_workers;
    return true;
}

bool GlobalSettings::setIsDeferredShaderCompilationOn(bool is_enabled)
{
    if (Initializer::isRendererInitialized())
        return false;

    m_deferred_shader_compilation = is_enabled;
    return true;
}

bool GlobalSettings::addShaderLookupDirectory(std::string const & path)
{
    if (Initializer::isRendererInitialized())
        return false;

    m_shader_lookup_directories.push_back(path);
    return true;
}

bool GlobalSettings::clearShaderLookupDirectories()
{
    if (Initializer::isRendererInitialized())
        return false;

    m_shader_lookup_directories.clear();
    return true;
}
