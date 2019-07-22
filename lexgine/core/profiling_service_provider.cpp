#include "lexgine/core/dx/d3d12/device.h"
#include "lexgine/core/concurrency/abstract_task.h"
#include "profiling_service_provider.h"

using namespace lexgine::core;

namespace {

std::chrono::nanoseconds getCurrentTime()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
}

}

CPUTaskProfilingService::CPUTaskProfilingService(std::string const& profiling_service_name)
    : m_name{ profiling_service_name }
    , m_color_uid{ dx::d3d12::pix_marker_colors::PixCPUJobMarkerColor }
    , m_last_tick{ getCurrentTime() }
{
}

void CPUTaskProfilingService::beginProfilingEvent()
{
    PIXBeginEvent(m_color_uid, m_name.c_str());
    m_last_tick = getCurrentTime();
}

void CPUTaskProfilingService::endProfilingEvent()
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



ProfilingServiceProvider::ProfilingServiceProvider(dx::d3d12::Device& device, bool enable_profiling)
    : m_device{ device }
    , m_is_profiling_enabled{ enable_profiling }
{
}

std::unique_ptr<ProfilingService> ProfilingServiceProvider::createService(concurrency::AbstractTask const& service_client) const
{
    return m_is_profiling_enabled ?
        std::make_unique<CPUTaskProfilingService>(service_client.getStringName() + "(CPU execution time)")
        : nullptr;
}

std::unique_ptr<ProfilingService> ProfilingServiceProvider::createService(dx::d3d12::tasks::rendering_tasks::GpuWorkExecutionTask const& service_client) const
{
    return nullptr;
}
