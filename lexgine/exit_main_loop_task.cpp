#include "exit_main_loop_task.h"

using namespace lexgine::core::concurrency;
using namespace lexgine::core::concurrency::tasks;

ExitMainLoopTask::ExitMainLoopTask():
    AbstractTask{ "Exit Main Loop" }
{
}

void ExitMainLoopTask::do_task(uint8_t worker_id)
{
}

TaskType ExitMainLoopTask::get_task_type() const
{
    return TaskType::exit;
}
