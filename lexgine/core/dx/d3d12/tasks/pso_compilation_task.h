#ifndef LEXGINE_CORE_DX_D3D12_TASKS_PSO_COMPILATION_TASK_H

#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/dx/d3d12/pipeline_state.h"
#include "lexgine/core/data_blob.h"
#include "lexgine/core/misc/static_vector.h"

#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "lexgine/core/dx/d3d12/task_caches/combined_cache_key.h"

#include <list>

namespace lexgine { namespace core { namespace dx { namespace d3d12 { namespace tasks {

//! Performs compilation and caching of graphics PSOs
class GraphicsPSOCompilationTask final : public concurrency::SchedulableTask
{
public:
    GraphicsPSOCompilationTask(task_caches::CombinedCacheKey const& key,
        GlobalSettings const& global_settings,
        Device& device,
        GraphicsPSODescriptor const& descriptor);

    PipelineState const& getTaskData() const;    //! returns blob containing compiled PSO
    bool wasSuccessful() const;    //! returns 'true' if the task has completed successfully
    bool execute(uint8_t worker_id);    //! executes the task manually and returns 'true' if the task does not require rescheduling

    void setVertexShaderCompilationTask(HLSLCompilationTask* vs_compilation_task);  
    HLSLCompilationTask* getVertexShaderCompilationTask() const;    
    
    void setHullShaderCompilationTask(HLSLCompilationTask* hs_compilation_task);   
    HLSLCompilationTask* getHullShaderCompilationTask() const;

    void setDomainShaderCompilationTask(HLSLCompilationTask* ds_compilation_task);  
    HLSLCompilationTask* getDomainShaderCompilationTask() const;

    void setGeometryShaderCompilationTask(HLSLCompilationTask* gs_compilation_task); 
    HLSLCompilationTask* getGeometryShaderCompilationTask() const;

    void setPixelShaderCompilationTask(HLSLCompilationTask* ps_compilation_task);
    HLSLCompilationTask* getPixelShaderCompilationTask() const;
    
    void setRootSignatureCompilationTask(RootSignatureCompilationTask* root_signature_compilation_task);
    RootSignatureCompilationTask* getRootSignatureCompilationTask() const;
    

private:
    // required by SchedulableTask

    bool do_task(uint8_t worker_id, uint16_t frame_index) override;
    concurrency::TaskType get_task_type() const override;

private:
    task_caches::CombinedCacheKey const& m_key;
    GlobalSettings const& m_global_settings;
    Device& m_device;
    GraphicsPSODescriptor m_descriptor;
    misc::StaticVector<HLSLCompilationTask*, 5> m_associated_shader_compilation_tasks;
    RootSignatureCompilationTask* m_associated_root_signature;
    bool m_was_successful; 
    std::unique_ptr<PipelineState> m_resulting_pipeline_state;
};


//! Performs compilation and caching of compute PSOs
class ComputePSOCompilationTask final : public concurrency::SchedulableTask
{
public:
    ComputePSOCompilationTask(task_caches::CombinedCacheKey const& key,
        GlobalSettings const& global_settings,
        Device& device,
        ComputePSODescriptor const& descriptor);

    PipelineState const& getTaskData() const;    //! returns blob containing compiled PSO data
    bool wasSuccessful() const;    //! returns 'true' if the task has completed successfully
    bool execute(uint8_t worker_id);    //! execute the task manually and returns 'true' on success

    void setComputeShaderCompilationTask(HLSLCompilationTask* cs_compilation_task);    //! sets compute shader compilation task associated with the PSO

    void setRootSignatureCompilationTask(RootSignatureCompilationTask* root_signature_compilation_task);    //! associates root signature compilation task with the PSO

private:
    // required by SchedulableTask

    bool do_task(uint8_t worker_id, uint16_t frame_index) override;
    concurrency::TaskType get_task_type() const override;

private:
    task_caches::CombinedCacheKey const& m_key;
    GlobalSettings const& m_global_settings;
    Device& m_device;
    ComputePSODescriptor m_descriptor;
    HLSLCompilationTask* m_associated_compute_shader;
    RootSignatureCompilationTask* m_associated_root_signature;
    bool m_was_successful;
    std::unique_ptr<PipelineState> m_resulting_pipeline_state;
};



}}}}}

#define LEXGINE_CORE_DX_D3D12_TASKS_PSO_COMPILATION_TASK_H
#endif
