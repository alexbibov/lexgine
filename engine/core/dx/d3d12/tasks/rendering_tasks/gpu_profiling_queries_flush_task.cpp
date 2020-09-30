#include "engine/core/globals.h"
#include "engine/core/global_settings.h"
#include "engine/core/dx/d3d12/query_cache.h"
#include "engine/core/dx/d3d12/device.h"

#include "gpu_profiling_queries_flush_task.h"

using namespace lexgine::core;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;

std::shared_ptr<GpuProfilingQueriesFlushTask> GpuProfilingQueriesFlushTask::create(Globals& globals)
{
    return std::shared_ptr<GpuProfilingQueriesFlushTask>{new GpuProfilingQueriesFlushTask{ globals }};
}

bool GpuProfilingQueriesFlushTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    m_cmd_list_ptr->reset();
    m_cmd_list_ptr->device().queryCache()->writeFlushCommandList(*m_cmd_list_ptr);
    m_cmd_list_ptr->close();
    return true;
}

GpuProfilingQueriesFlushTask::GpuProfilingQueriesFlushTask(Globals& globals)
    : RenderingWork{ globals, "Flush GPU profiling queries", CommandType::direct, false }
    , m_cmd_list_ptr{ addCommandList() }
{

}
