#ifndef LEXGINE_CORE_GLOBAL_SETTINGS_H
#define LEXGINE_CORE_GLOBAL_SETTINGS_H

#include <cstdint>
#include <string>
#include <vector>

namespace lexgine { namespace core { 


//! Encapsulates global settings of the engine
class GlobalSettings
{
private:

    uint8_t m_number_of_workers;
    bool m_deferred_shader_compilation;
    std::vector<std::string> m_shader_lookup_directories;
    std::string m_cache_path;

public:
    GlobalSettings() = default;
    GlobalSettings(std::string const& json_settings_source_path);

    void serialize(std::string const& json_serialization_path) const;


    // *** the following functions are used to retrieve global settings ***

    uint8_t getNumberOfWorkers() const;
    bool isDeferredShaderCompilationOn() const;
    std::vector<std::string> const& getShaderLookupDirectories() const;
    std::string getCacheDirectory() const;

    
    // *** the following functions are used to alter the global settings during run time. All functions return 'true' in case of success and 'false' if the parameter's value cannot be changed ***
    
    bool setNumberOfWorkers(uint8_t num_workers);
    bool setIsDeferredShaderCompilationOn(bool is_enabled);
    bool addShaderLookupDirectory(std::string const& path);
    bool clearShaderLookupDirectories();
    bool setCacheDirectory(std::string const& path);
};

}}

#endif
