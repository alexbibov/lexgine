#include "data_uploading_task.h"
#include "lexgine/core/dx/d3d12/heap_data_uploader.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12::tasks;

DataUploadingTask::DataUploadingTask(Globals const& globals):
    m_globals{ globals }
{

}


DataUploadingTask::~DataUploadingTask() = default;