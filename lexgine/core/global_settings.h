#ifndef LEXGINE_CORE_GLOBAL_SETTINGS_H
#define LEXGINE_CORE_GLOBAL_SETTINGS_H

#include <cstdint>
#include <string>

namespace lexgine { namespace core { 

//! Encapsulates global settings of the engine
class GlobalSettings
{
private:

    uint8_t m_number_of_workers = 8U;
    bool m_deferred_shader_compilation = true;


public:
    GlobalSettings() = default;
    GlobalSettings(std::string const& json_settings_source_path);

    void serialize(std::string const& json_serialization_path) const;

    uint8_t getNumberOfWorkers() const;
    bool isDeferredShaderCompilationOn() const;
};

}}

#endif
