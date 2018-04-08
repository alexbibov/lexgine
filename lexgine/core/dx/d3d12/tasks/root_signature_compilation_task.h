#ifndef LEXGINE_CORE_DX_D3D12_TASKS_ROOT_SIGNATURE_COMPILATION_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_ROOT_SIGNATURE_COMPILATION_TASK_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/root_signature.h"
#include "lexgine/core/dx/d3d12/task_caches/combined_cache_key.h"

namespace lexgine { namespace core { namespace dx { namespace d3d12 { namespace tasks {

class RootSignatureCompilationTask : public concurrency::SchedulableTask
{
public:
    RootSignatureCompilationTask(task_caches::CombinedCacheKey const& key,
        GlobalSettings const& global_settings,
        RootSignature&& root_signature, RootSignatureFlags const& flags);

    D3DDataBlob const& getTaskData() const;    //! returns D3D data blob containing compiled root signature
    bool wasSuccessful() const;    //! returns 'true' if the task has completed successfully
    bool execute(uint8_t worker_id);    //! executes the task manually and returns 'true' if the task does not require rescheduling

public:
    // required by SchedulableTask interface

    bool do_task(uint8_t worker_id, uint16_t frame_index) override;
    concurrency::TaskType get_task_type() const override;

private:
    task_caches::CombinedCacheKey const& m_key;
    GlobalSettings const& m_global_settings;
    RootSignature m_rs;
    RootSignatureFlags m_rs_flags;
    bool m_was_successful;
    D3DDataBlob m_compiled_rs_blob;
};

}}}}}

#endif
