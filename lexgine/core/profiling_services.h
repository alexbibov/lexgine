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
#include "lexgine/core/dx/d3d12/query_cache.h"
#include "lexgine/core/dx/d3d12/command_allocator_ring.h"

namespace lexgine::core {

enum class ProfilingServiceType
{
    cpu_work_timestamp, 
    gpu_graphics_work_timestamp,
    gpu_compute_work_timestamp,
    gpu_copy_work_timestamp,
    gpu_general_work_timestamp,
    other,
    count
};

class ProfilingService
{
public:
    static size_t constexpr c_statistics_package_length = dx::d3d12::QueryCache::c_statistics_package_length;
    using statistics_t = std::array<double, c_statistics_package_length>;

public:
    ProfilingService(GlobalSettings const& settings, std::string const& service_string_name);

    std::string name() const { return m_name; };    //! user-friendly name of the profiling event
    statistics_t const& statistics();    //! statistics associated with the event

    virtual uint32_t uid() const = 0;    //! 4-byte UID of profiling event, which can be also used for color coding in UI
    virtual double timingFrequency() const = 0;    //! retrieves timing frequency of the profiling service

    void beginProfilingEvent();    //! starts new profiling event
    void endProfilingEvent();    //! ends the last profiling event started and updates the profiling statistics

    //! Initializes all profiling services created so far. This function must be called before any actual interaction with the services.
    static void initializeProfilingServices(Globals& globals);

    //! Converts profiling service UID to profiling service type enumeration
    ProfilingServiceType serviceType() const;

private:
    virtual void beginProfilingEventImpl(bool global_profiling_enabled) = 0;
    virtual void endProfilingEventImpl(bool global_profiling_enabled) = 0;
    virtual statistics_t const& statisticsImpl() { return m_statistics; }

protected:
    GlobalSettings const& m_settings;
    std::string m_name;
    statistics_t m_statistics;
};

class CPUTaskProfilingService : public ProfilingService
{
public:
    CPUTaskProfilingService(GlobalSettings const& settings, std::string const& profiling_service_name);

    virtual uint32_t uid() const override { return dx::d3d12::pix_marker_colors::PixCPUJobMarkerColor; }
    virtual double timingFrequency() const override { return m_frequency; }

private:
    void beginProfilingEventImpl(bool global_profiling_enabled) override;
    void endProfilingEventImpl(bool global_profiling_enabled) override;

private:
    static double constexpr m_frequency{ 1e6 };
    std::chrono::nanoseconds m_last_tick;
    int m_spin_up_period_counter{ 0 };
};



class GPUTaskProfilingService : public ProfilingService
{
public:
    GPUTaskProfilingService(GlobalSettings const& settings, std::string const& service_string_name);

    uint32_t uid() const override;
    double timingFrequency() const override;

    void assignCommandLists(std::list<dx::d3d12::CommandList>& command_lists_to_patch,
        dx::d3d12::Device const& device, dx::d3d12::CommandType gpu_work_type);

private:
    void beginProfilingEventImpl(bool global_profiling_enabled) override;
    void endProfilingEventImpl(bool global_profiling_enabled) override;
    virtual statistics_t const& statisticsImpl() override;

private:
    dx::d3d12::Device const* m_device_ptr{ nullptr };
    dx::d3d12::QueryCache* m_query_cache_ptr{ nullptr };
    int m_spin_up_period_counter{ -1 };
    std::list<dx::d3d12::CommandList>* m_command_lists_to_patch_ptr{ nullptr };
    dx::d3d12::CommandType m_gpu_work_pack_type;

    dx::d3d12::QueryHandle m_timestamp_query;
};



}

#endif
