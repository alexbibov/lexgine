#include <chrono>

#include "lexgine/core/profiling_service_provider.h"
#include "lexgine/core/dx/d3d12/pix_support.h"
#include "abstract_task.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::concurrency;


AbstractTask::AbstractTask(ProfilingServiceProvider const* p_profiling_service_provider,
    std::string const& debug_name, bool expose_in_task_graph)
    : m_profiling_service_provider_ptr{ p_profiling_service_provider }
    , m_exposed_in_task_graph{ expose_in_task_graph }
    
{
    if (debug_name.length())
        setStringName(debug_name);
}

AbstractTask::~AbstractTask() = default;

bool AbstractTask::execute(uint8_t worker_id, uint64_t user_data)
{
    if (m_profiling_service_provider_ptr)
    {
        if(m_profiling_service_provider_ptr->isProfilingEnabled())
        {
            if(!m_profiling_service)
                m_profiling_service = m_profiling_service_provider_ptr->createService(*this);
        }
        else
        {
            m_profiling_service = nullptr;
        }
    }

    if (m_profiling_service)
    {
        m_profiling_service->beginProfilingEvent();
    }

    bool result = doTask(worker_id, user_data);

    if (m_profiling_service)
    {
        m_profiling_service->endProfilingEvent();
    }

    return result;
}

std::unique_ptr<ProfilingService> AbstractTask::createProfilingService() const
{
    return m_profiling_service_provider_ptr->createService(*this);
}
