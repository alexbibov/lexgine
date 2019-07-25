#include <chrono>

#include "lexgine/core/profiling_service_provider.h"
#include "lexgine/core/dx/d3d12/pix_support.h"
#include "abstract_task.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::concurrency;


AbstractTask::AbstractTask(std::string const& debug_name, bool expose_in_task_graph,
    std::unique_ptr<ProfilingService>&& profiling_service)
    : m_profiling_service{ std::move(profiling_service) }
    , m_exposed_in_task_graph{ expose_in_task_graph }

{
    if (debug_name.length())
        setStringName(debug_name);
}

AbstractTask::~AbstractTask() = default;

bool AbstractTask::execute(uint8_t worker_id, uint64_t user_data)
{
    if (m_profiling_service) m_profiling_service->beginProfilingEvent();
    bool result = doTask(worker_id, user_data);
    if (m_profiling_service) m_profiling_service->endProfilingEvent();

    return result;
}
