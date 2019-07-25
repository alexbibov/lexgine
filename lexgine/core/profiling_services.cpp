#include "lexgine/core/dx/d3d12/device.h"
#include "lexgine/core/concurrency/abstract_task.h"
#include "lexgine/core/global_settings.h"

#include "profiling_services.h"

using namespace lexgine::core;

namespace {

std::chrono::nanoseconds getCurrentTime()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
}

}

ProfilingService::ProfilingService(GlobalSettings const& settings, std::string const& service_string_name)
    : m_settings{ settings }
    , m_name{ service_string_name }
{
}

void ProfilingService::beginProfilingEvent()
{
    if (m_settings.isProfilingEnabled())
        beginProfilingEventImpl();
}

void ProfilingService::endProfilingEvent()
{
    if (m_settings.isProfilingEnabled())
        endProfilingEventImpl();
}



CPUTaskProfilingService::CPUTaskProfilingService(GlobalSettings const& settings, std::string const& profiling_service_name)
    : ProfilingService{ settings, profiling_service_name }
    , m_last_tick{ getCurrentTime() }
{
}

void CPUTaskProfilingService::beginProfilingEventImpl()
{
    PIXBeginEvent(colorUID(), m_name.c_str());
    m_last_tick = getCurrentTime();
}

void CPUTaskProfilingService::endProfilingEventImpl()
{
    float conversion_coefficient = std::nano::num * m_frequency / std::nano::den;

    auto time_delta = getCurrentTime() - m_last_tick;

    if (m_spin_up_period_counter < c_statistics_package_length)
    {
        m_statistics[m_spin_up_period_counter++] = time_delta.count() * conversion_coefficient;
    }
    else
    {
        for (int i = 0; i < c_statistics_package_length - 1; ++i)
            m_statistics[i] = m_statistics[i + 1];
        m_statistics[c_statistics_package_length - 1] = time_delta.count() * conversion_coefficient;
    }

    PIXEndEvent();
}