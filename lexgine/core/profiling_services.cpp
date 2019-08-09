#include <cassert>

#include "lexgine/core/concurrency/abstract_task.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/dx/d3d12/device.h"
#include "lexgine/core/dx/d3d12/command_list.h"
#include "lexgine/core/dx/d3d12/query_cache.h"

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
    , m_statistics{ 0 }
{
}

ProfilingService::statistics_t const& ProfilingService::statistics()
{
    if (m_settings.isProfilingEnabled())
        return statisticsImpl();
    else
        return m_statistics;
}

void ProfilingService::beginProfilingEvent()
{
    beginProfilingEventImpl(m_settings.isProfilingEnabled());
}

void ProfilingService::endProfilingEvent()
{
    endProfilingEventImpl(m_settings.isProfilingEnabled());
}

void ProfilingService::initializeProfilingServices(Globals& globals)
{
    globals.get<dx::d3d12::Device>()->queryCache()->initQueryCache();
}



CPUTaskProfilingService::CPUTaskProfilingService(GlobalSettings const& settings, std::string const& profiling_service_name)
    : ProfilingService{ settings, profiling_service_name }
    , m_last_tick{ getCurrentTime() }
{
}

void CPUTaskProfilingService::beginProfilingEventImpl(bool profiler_enabled)
{
    if (profiler_enabled)
    {
        PIXBeginEvent(colorUID(), m_name.c_str());
        m_last_tick = getCurrentTime();
    }
}

void CPUTaskProfilingService::endProfilingEventImpl(bool profiler_enabled)
{
    if (profiler_enabled)
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
}



GPUTaskProfilingService::GPUTaskProfilingService(GlobalSettings const& settings, std::string const& service_string_name)
    : ProfilingService{ settings, service_string_name }
{

}

ProfilingService::statistics_t const& GPUTaskProfilingService::statisticsImpl()
{
    if (m_spin_up_period_counter < 0)
    {
        m_spin_up_period_counter = 0;
    }
    else
    {
        uint64_t const* p_query_data = static_cast<uint64_t const*>(m_query_cache_ptr->fetchQuery(m_timestamp_query));
        uint64_t begin = *p_query_data;
        uint64_t end = *(p_query_data + 1);
        uint64_t tick_count = begin <= end ? end - begin : 0;

        float time_delta = tick_count / timingFrequency() * 1e3f;

        if (m_spin_up_period_counter < c_statistics_package_length)
        {
            m_statistics[m_spin_up_period_counter++] = time_delta;
        }
        else
        {
            for (int i = 0; i < c_statistics_package_length - 1; ++i)
                m_statistics[i] = m_statistics[i + 1];
            m_statistics[c_statistics_package_length - 1] = time_delta;
        }
    }

    return m_statistics;
}

uint32_t GPUTaskProfilingService::colorUID() const
{
    if (!m_command_lists_to_patch_ptr) return dx::d3d12::pix_marker_colors::PixGPUGeneralJobColor;

    switch (m_gpu_work_pack_type)
    {
    case dx::d3d12::CommandType::direct:
        return dx::d3d12::pix_marker_colors::PixGPUGraphicsJobMarkerColor;

    case dx::d3d12::CommandType::compute:
        return dx::d3d12::pix_marker_colors::PixGPUComputeJobMarkerColor;

    case dx::d3d12::CommandType::copy:
        return dx::d3d12::pix_marker_colors::PixGPUCopyJobMarkerColor;

    default:
        assert(false);
        __assume(0);
    }
}

float GPUTaskProfilingService::timingFrequency() const
{
    if (!m_command_lists_to_patch_ptr) return 0.f;

    switch (m_gpu_work_pack_type)
    {
    case dx::d3d12::CommandType::direct:
        return static_cast<float>(m_device_ptr->defaultCommandQueue().getTimeStampFrequency());

    case dx::d3d12::CommandType::compute:
        return static_cast<float>(m_device_ptr->asyncCommandQueue().getTimeStampFrequency());

    case dx::d3d12::CommandType::copy:
        return static_cast<float>(m_device_ptr->copyCommandQueue().getTimeStampFrequency());

    default:
        assert(false);
        __assume(0);
    }
}

void GPUTaskProfilingService::assignCommandLists(std::list<dx::d3d12::CommandList>& command_lists_to_patch,
    dx::d3d12::Device const& device, dx::d3d12::CommandType gpu_work_type)
{
    m_command_lists_to_patch_ptr = &command_lists_to_patch;
    m_device_ptr = &device;
    m_query_cache_ptr = device.queryCache();
    m_gpu_work_pack_type = gpu_work_type;

    switch (m_gpu_work_pack_type)
    {
    case dx::d3d12::CommandType::direct:
    case dx::d3d12::CommandType::compute:
        m_timestamp_query = m_query_cache_ptr->registerTimestampQuery(2);
        break;
    case dx::d3d12::CommandType::copy:
        m_timestamp_query = m_query_cache_ptr->registerCopyQueueTimestampQuery(2);
        break;
    default:
        assert(false);
        __assume(0);
    }
}


void GPUTaskProfilingService::beginProfilingEventImpl(bool profiler_enabled)
{
    for (auto& cmd_list : *m_command_lists_to_patch_ptr)
    {
        cmd_list.reset();
    }

    if (profiler_enabled)
    {
        PIXBeginEvent(m_command_lists_to_patch_ptr->front().native().Get(), colorUID(), name().c_str());
        m_command_lists_to_patch_ptr->front().endQuery(m_timestamp_query);
    }
}

void GPUTaskProfilingService::endProfilingEventImpl(bool profiler_enabled)
{
    if (profiler_enabled)
    {
        m_command_lists_to_patch_ptr->back().endQuery(m_timestamp_query);
        PIXEndEvent(m_command_lists_to_patch_ptr->back().native().Get());
    }

    for (auto& cmd_list : *m_command_lists_to_patch_ptr)
    {
        cmd_list.close();
    }
}