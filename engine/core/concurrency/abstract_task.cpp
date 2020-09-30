#include <chrono>

#include "engine/core/profiling_services.h"
#include "engine/core/dx/d3d12/pix_support.h"

#include "abstract_task.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::concurrency;


AbstractTask::AbstractTask(std::string const& debug_name, bool expose_in_task_graph)
    : m_exposed_in_task_graph{ expose_in_task_graph }

{
    if (debug_name.length())
        setStringName(debug_name);
}

AbstractTask::~AbstractTask() = default;

bool AbstractTask::execute(uint8_t worker_id, uint64_t user_data)
{
    for (auto& ps : m_profiling_services)
    {
        ps->beginProfilingEvent();
    }
    
    bool result = doTask(worker_id, user_data);

    for (auto& ps : m_profiling_services)
    {
        ps->endProfilingEvent();
    }

    return result;
}