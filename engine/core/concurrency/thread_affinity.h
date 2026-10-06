#ifndef LEXGINE_CORE_CONCURRENCY_THREAD_AFFINITY_H
#define LEXGINE_CORE_CONCURRENCY_THREAD_AFFINITY_H

#include <cassert>


namespace lexgine::core::concurrency {

/*! Claims the simulation role for the calling thread. The simulation state must only be touched from a
 thread holding that role
*/
void registerSimulationThread();

/*! Claims the rendering role for the calling thread. The role is held by every thread doing rendering work:
 the thread orchestrating the frames as well as each worker recording the command lists
*/
void registerRenderThread();

//! returns 'true' if the calling thread holds the simulation role
bool isOnSimulationThread();

//! returns 'true' if the calling thread holds the rendering role
bool isOnRenderThread();

//! asserts in the debug builds that the caller holds the simulation role. Compiles to nothing otherwise
inline void assertOnSimulationThread()
{
    assert(isOnSimulationThread());
}

//! asserts in the debug builds that the caller holds the rendering role. Compiles to nothing otherwise
inline void assertOnRenderThread()
{
    assert(isOnRenderThread());
}

}

#endif
