#ifndef LEXGINE_CORE_DX_D3D12_TASKS_PSO_COMPILATION_TASK_H

#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/dx/d3d12/pipeline_state.h"
#include "lexgine/core/data_blob.h"
#include "lexgine/core/misc/static_vector.h"

#include "lexgine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "lexgine/core/dx/d3d12/task_caches/pso_compilation_task_cache.h"

#include <list>

namespace lexgine { namespace core { namespace dx { namespace d3d12 { namespace tasks {

//! Abstraction for different kind of PSO compilation tasks
class PSOCompilationTask : public concurrency::SchedulableTask
{
public:
    virtual D3DDataBlob getTaskData() const = 0;    //! returns blob containing compiled PSO
    virtual bool wasSuccessful() const = 0;    //! returns 'true' if the task has completed successfully
    virtual bool execute(uint64_t worker_id) = 0;    //! manual execution of the task; returns 'true' on success
};


//! Performs compilation and caching of graphics PSOs
class GraphicsPSOCompilationTask final : public PSOCompilationTask
{
public:
    GraphicsPSOCompilationTask(GraphicsPSODescriptor const& descriptor, 
        task_caches::PSOCompilationTaskCache::Key const& key);

    D3DDataBlob getTaskData() const override;    //! returns blob containing compiled PSO
    bool wasSuccessful() const override;    //! returns 'true' if the task has completed successfully
    bool execute(uint64_t worker_id) override;    //! executes the task manually and returns 'true' on success

    void setVertexShaderCompilationTask(HLSLCompilationTask* vs_compilation_task);    //! sets vertex shader compilation task associated with the PSO
    void setHullShaderCompilationTask(HLSLCompilationTask* hs_compilation_task);    //! sets hull shader compilation task associated with the PSO
    void setDomainShaderCompilationTask(HLSLCompilationTask* ds_compilation_task);    //! sets domain shader compilation task associated with the PSO
    void setGeometryShaderCompilationTask(HLSLCompilationTask* gs_compilation_task);    //! sets geometry shader compilation task associated with the PSO
    void setPixelShaderCompilationTask(HLSLCompilationTask* ps_compilation_task);    //! sets pixel shader compilation task associated with the PSO

private:
    // required by SchedulableTask

    bool do_task(uint8_t worker_id, uint16_t frame_index) override;
    concurrency::TaskType get_task_type() const override;

private:
    GraphicsPSODescriptor m_descriptor;
    task_caches::PSOCompilationTaskCache::Key m_key;
    misc::StaticVector<HLSLCompilationTask*, 5> m_associated_shader_compilation_tasks;
};


//! Performs compilation and caching of compute PSOs
class ComputePSOCompilationTask final : PSOCompilationTask
{
public:
    ComputePSOCompilationTask(ComputePSODescriptor const& descriptor,
        task_caches::PSOCompilationTaskCache::Key const& key);

    D3DDataBlob getTaskData() const override;    //! returns blob containing compiled PSO data
    bool wasSuccessful() const override;    //! returns 'true' if the task has completed successfully
    bool execute(uint64_t worker_id) override;    //! execute the task manually and returns 'true' on success

    void setComputeShaderCompilationTask(HLSLCompilationTask* cs_compilation_task);    //! sets compute shader compilation task associated with the PSO

private:
    // required by SchedulableTask

    bool do_task(uint8_t worker_id, uint16_t frame_index) override;
    concurrency::TaskType get_task_type() const override;

private:
    ComputePSODescriptor m_descriptor;
    task_caches::PSOCompilationTaskCache::Key m_key;
    HLSLCompilationTask* m_associated_compute_shader;
};



}}}}}

#define LEXGINE_CORE_DX_D3D12_TASKS_PSO_COMPILATION_TASK_H
#endif
