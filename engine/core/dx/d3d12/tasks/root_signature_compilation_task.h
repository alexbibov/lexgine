#ifndef LEXGINE_CORE_DX_D3D12_TASKS_ROOT_SIGNATURE_COMPILATION_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_ROOT_SIGNATURE_COMPILATION_TASK_H

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/concurrency/schedulable_task.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/root_signature.h"
#include "engine/core/dx/d3d12/task_caches/combined_cache_key.h"

namespace lexgine::core::dx::d3d12::tasks {

class RootSignatureCompilationTask : public concurrency::SchedulableTask
{
public:
    RootSignatureCompilationTask(task_caches::CombinedCacheKey const& key,
        Globals const& globals, RootSignature&& root_signature, RootSignatureFlags const& flags,
        misc::DateTime const& timestamp);

    D3DDataBlob const& getTaskData() const;    //! returns D3D data blob containing compiled root signature
    bool wasSuccessful() const;    //! returns 'true' if the task has completed successfully
    bool execute(uint8_t worker_id);    //! executes the task manually and returns 'true' if the task does not require rescheduling
    
    /*! returns string name associated with the root signature in root signature compilation task cache
        The names are required to follow special convention (note the '__ROOTSIGNATURE' suffix):

            <user_defined_string_name><user_defined_numeric_id>__ROOTSIGNATURE

        For example: forward_illumination_unshadowed_pass00__ROOTSIGNATURE
    */
    std::string getCacheName() const;    

public:
    // required by SchedulableTask interface

    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override;

private:
    task_caches::CombinedCacheKey const& m_key;
    GlobalSettings const& m_global_settings;
    RootSignature m_rs;
    RootSignatureFlags m_rs_flags;
    bool m_was_successful;
    D3DDataBlob m_compiled_rs_blob;
    misc::DateTime m_timestamp;
};

}

#endif
