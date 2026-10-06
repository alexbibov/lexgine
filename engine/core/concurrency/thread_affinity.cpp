#include <thread>

#include "thread_affinity.h"


namespace lexgine::core::concurrency {

namespace {

// a thread, which was never registered, matches neither role: a missed registration has to surface rather
// than make every affirmation vacuous
std::thread::id g_simulation_thread_id{};
std::thread::id g_render_thread_id{};

}  // namespace

void registerSimulationThread()
{
    g_simulation_thread_id = std::this_thread::get_id();
}

void registerRenderThread()
{
    g_render_thread_id = std::this_thread::get_id();
}

bool isOnSimulationThread()
{
    return g_simulation_thread_id != std::thread::id{}
        && std::this_thread::get_id() == g_simulation_thread_id;
}

bool isOnRenderThread()
{
    return g_render_thread_id != std::thread::id{}
        && std::this_thread::get_id() == g_render_thread_id;
}

}
