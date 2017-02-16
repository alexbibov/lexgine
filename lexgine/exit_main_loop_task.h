#ifndef LEXGINE_CORE_CONCURRENCY_TASKS_EXIT_MAIN_LOOP_TASK_H

#include "task.h"

namespace lexgine {namespace core {namespace concurrency {namespace tasks {

class ExitMainLoopTask : public AbstractTask
{
public:
    ExitMainLoopTask();

private:
    TaskType get_task_type() const override;
};

}}}}

#define LEXGINE_CORE_CONCURRENCY_TASKS_EXIT_MAIN_LOOP_TASK_H
#endif
