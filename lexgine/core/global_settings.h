#ifndef LEXGINE_CORE_GLOBAL_SETTINGS_H
#define LEXGINE_CORE_GLOBAL_SETTINGS_H

#include <cstdint>

namespace lexgine { namespace core { 

/*! Encapsulated global settings of the engine. This class is not thread-safe and
 it is assumed that there is just one instance of this class regardless of the actual number of threads
*/
class GlobalSettings
{
private:
    static GlobalSettings* m_p_self;

    uint8_t m_number_of_workers = 8U;
    bool m_deferred_shader_compilation = true;


    GlobalSettings();

    GlobalSettings(GlobalSettings const&) = delete;
    GlobalSettings(GlobalSettings&&) = delete;

    GlobalSettings& operator=(GlobalSettings const&) = delete;
    GlobalSettings& operator=(GlobalSettings&&) = delete;


public:
    static GlobalSettings* initialize();
    static void destroy();

    uint8_t getNumberOfWorkers() const;

    bool deferredShaderCompilation() const;
};

}}

#endif
