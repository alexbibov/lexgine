#ifndef LEXGINE_CORE_PROFILING_SERVICES_H
#define LEXGINE_CORE_PROFILING_SERVICES_H

#include <memory>
#include <string>
#include <array>
#include <chrono>
#include <unordered_map>

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/concurrency/lexgine_core_concurrency_fwd.h"
#include "lexgine/core/dx/d3d12/tasks/rendering_tasks/lexgine_core_dx_d3d12_tasks_rendering_tasks_fwd.h"
#include "lexgine/core/dx/d3d12/pix_support.h"


namespace lexgine::core {


class ProfilingService
{
public:
    static size_t constexpr c_statistics_package_length = 10;
    using statistics_t = std::array<float, c_statistics_package_length>;

public:
    ProfilingService(GlobalSettings const& settings, std::string const& service_string_name);

    std::string name() const { return m_name; };    //! user-friendly name of the profiling event
    virtual uint32_t colorUID() const = 0;    //! 4-byte UID of profiling event, which can be also used for color coding in UI
    virtual float timingFrequency() const = 0;    //! retrieves timing frequency of the profiling service
    virtual statistics_t const& statistics() const = 0;    //! statistics associated with the event

    void beginProfilingEvent();    //! starts new profiling event
    void endProfilingEvent();    //! ends the last profiling event started and updates the profiling statistics

private:
    virtual void beginProfilingEventImpl() = 0;
    virtual void endProfilingEventImpl() = 0;

protected:
    GlobalSettings const& m_settings;
    std::string m_name;
    statistics_t m_statistics;
};

class CPUTaskProfilingService : public ProfilingService
{
public:
    CPUTaskProfilingService(GlobalSettings const& settings, std::string const& profiling_service_name);

    virtual uint32_t colorUID() const override { return dx::d3d12::pix_marker_colors::PixCPUJobMarkerColor; }
    virtual statistics_t const& statistics() const override { return m_statistics; }
    virtual float timingFrequency() const override { return m_frequency; }

private:
    virtual void beginProfilingEventImpl() override;
    virtual void endProfilingEventImpl() override;

private:
    static float constexpr m_frequency{ 1e6f };
    std::chrono::nanoseconds m_last_tick;
    int m_spin_up_period_counter{ 0 };
};

class GPUTaskProfilingServiceTimestamp : public ProfilingService
{
public:
    GPUTaskProfilingServiceTimestamp(Globals const& globals, dx::d3d12::CommandList& command_list);
};



}

#endif
