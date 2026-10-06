#include "thread_affinity.h"


namespace lexgine::core::concurrency {

namespace {

// a thread, which claimed no role, holds none of them: a missed registration has to surface rather than
// make every affirmation vacuous
thread_local bool t_holds_simulation_role = false;
thread_local bool t_holds_render_role = false;

}  // namespace

void registerSimulationThread()
{
    t_holds_simulation_role = true;
}

void registerRenderThread()
{
    t_holds_render_role = true;
}

bool isOnSimulationThread()
{
    return t_holds_simulation_role;
}

bool isOnRenderThread()
{
    return t_holds_render_role;
}

}
