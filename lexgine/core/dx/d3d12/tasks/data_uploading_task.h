#ifndef LEXGINE_CORE_DX_D3D12_TASKS_DATA_UPLOADING_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_DATA_UPLOADING_TASK_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/concurrency/schedulable_task.h"

namespace lexgine { namespace core { namespace dx { namespace d3d12 { namespace tasks {

class DataUploadingTask final : public concurrency::SchedulableTask
{
public:
    DataUploadingTask(Globals const& globals);
    ~DataUploadingTask();

private:
    // required by SchedulableTask

    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override;

private:
    Globals const& m_globals;
    std::unique_ptr<ResourceDataUploader> m_data_uploader;
};


}}}}}

#endif