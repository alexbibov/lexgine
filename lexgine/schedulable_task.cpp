#include "schedulable_task.h"

using namespace lexgine::core::concurrency;

SchedulableTask::SchedulableTask(std::string const & debug_name):
    AbstractTask{ debug_name },
    TaskGraphNode{ static_cast<AbstractTask&>(*this) }
{
}
