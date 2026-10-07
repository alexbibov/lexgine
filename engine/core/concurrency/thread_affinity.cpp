#include "thread_affinity.h"


namespace lexgine::core::concurrency {

namespace {

// a thread, which claimed no role, holds none of them: a missed registration has to surface rather than
// make every affirmation vacuous
thread_local bool t_holds_simulation_role = false;
thread_local bool t_holds_master_worker_role = false;
thread_local bool t_holds_slave_worker_role = false;

}  // namespace

void registerSimulationThread()
{
    t_holds_simulation_role = true;
}

void registerMasterWorkerThread()
{
    t_holds_master_worker_role = true;
}

void registerSlaveWorkerThread()
{
    t_holds_slave_worker_role = true;
}

bool isOnSimulationThread()
{
    return t_holds_simulation_role;
}

bool isOnMasterWorkerThread()
{
    return t_holds_master_worker_role;
}

bool isOnSlaveWorkerThread()
{
    return t_holds_slave_worker_role;
}

bool isOnRenderThread()
{
    return t_holds_master_worker_role || t_holds_slave_worker_role;
}

}
