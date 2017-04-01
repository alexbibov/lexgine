#ifndef LEXGINE_CORE_DX_D3D12_TASKS_PSO_COMPILATION_TASK_H

#include "schedulable_task.h"
#include "pipeline_state.h"
#include "data_blob.h"

#include <list>

namespace lexgine {namespace core {namespace dx {namespace d3d12 {namespace tasks {


//! Implements compilation of provided HLSL source code
class PSOCompilationTask : public concurrency::SchedulableTask
{
public:
    PSOCompilationTask(GraphicsPSODescriptor desc, std::string const& pso_cache_name);
    PSOCompilationTask(ComputePSODescriptor desc, std::string const& pso_cache_name);


    D3DDataBlob getTaskData() const;

private:
    bool do_task(uint8_t worker_id, uint16_t frame_index) override;    //! performs actual compilation of the shader
    concurrency::TaskType get_task_type() const override;    //! returns type of this task (CPU)

    union PSODescriptor
    {
        GraphicsPSODescriptor graphics;
        ComputePSODescriptor compute;
    } m_pso_descriptor;

    enum class PSOType 
    { 
        graphics, 
        compute 
    } m_pso_type;
};

}}}}}

#define LEXGINE_CORE_DX_D3D12_TASKS_PSO_COMPILATION_TASK_H
#endif
