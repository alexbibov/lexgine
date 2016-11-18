#ifndef LEXGINE_CORE_CONCURRENCY_TASK_SINK_H

#include "task.h"

#include <list>
#include <memory>

namespace lexgine {namespace core {namespace concurrency {

//! Task queue divided between given number of workers
class TaskSink
{
public:
    TaskSink(uint8_t num_workers = 8U);

private:


};

}}}

#define LEXGINE_CORE_CONCURRENCY_TASK_SINK_H
#endif
