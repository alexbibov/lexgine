#include "global_settings.h"

using namespace lexgine::core;


GlobalSettings* GlobalSettings::m_p_self{ nullptr };


GlobalSettings::GlobalSettings()
{

}

GlobalSettings * lexgine::core::GlobalSettings::initialize()
{
    if (!m_p_self) m_p_self = new GlobalSettings{};
    return m_p_self;
}

void GlobalSettings::destroy()
{
    if (m_p_self)
    {
        delete m_p_self;
        m_p_self = nullptr;
    }
}

uint8_t GlobalSettings::getNumberOfWorkers() const
{
    return m_number_of_workers;
}

bool GlobalSettings::deferredShaderCompilation() const
{
    return m_deferred_shader_compilation;
}
