#ifndef LEXGINE_CORE_PROFILING_SERVICE_PROVIDER_H
#define LEXGINE_CORE_PROFILING_SERVICE_PROVIDER_H

#include <memory>
#include <string>
#include <array>
#include <chrono>
#include <unordered_map>

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
    virtual std::string name() const = 0;    //! user-friendly name of the profiling event
    virtual uint32_t colorUID() const = 0;    //! 4-byte UID of profiling event, which can be also used for color coding in UI
    virtual float timingFrequency() const = 0;    //! retrieves timing frequency of the profiling service
    virtual statistics_t const& statistics() const = 0;    //! statistics associated with the event

    virtual void beginProfilingEvent() = 0;    //! starts new profiling event
    virtual void endProfilingEvent() = 0;    //! ends the last profiling event started and updates the profiling statistics

protected:
    statistics_t m_statistics;
};

class CPUTaskProfilingService : public ProfilingService
{
public:
    CPUTaskProfilingService(std::string const& profiling_service_name);

    virtual std::string name() const override { return m_name; }
    virtual uint32_t colorUID() const override { return m_color_uid; }
    virtual statistics_t const& statistics() const override { return m_statistics; }
    virtual float timingFrequency() const override { return m_frequency; }

    virtual void beginProfilingEvent() override;
    virtual void endProfilingEvent() override;

private:
    std::string const m_name;
    uint32_t const m_color_uid;
    static float constexpr m_frequency{ 1e6f };
    std::chrono::nanoseconds m_last_tick;
    int m_spin_up_period_counter{ 0 };
};

class GPUTaskProfilingService : public ProfilingService
{
public:

};


class ProfilingServiceProvider final
{
public:
    ProfilingServiceProvider(dx::d3d12::Device& device, bool enable_profiling);

    std::unique_ptr<ProfilingService> createService(concurrency::AbstractTask const& service_client) const;
    std::unique_ptr<ProfilingService> createService(dx::d3d12::tasks::rendering_tasks::GpuWorkExecutionTask const& service_client) const;

    bool isProfilingEnabled() const { return m_is_profiling_enabled; }
    void toggleProfilingEnableState(bool enable_state) { m_is_profiling_enabled = !m_is_profiling_enabled; }

private:
    dx::d3d12::Device& m_device;
    bool m_is_profiling_enabled;
};



}

#endif
