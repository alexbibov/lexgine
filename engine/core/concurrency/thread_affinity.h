#ifndef LEXGINE_CORE_CONCURRENCY_THREAD_AFFINITY_H
#define LEXGINE_CORE_CONCURRENCY_THREAD_AFFINITY_H

#include <cassert>


namespace lexgine::core::concurrency {

/*! Records the calling thread as the one driving the simulation. The engine state owned by the simulation
 must only be touched from that thread
*/
void registerSimulationThread();

/*! Records the calling thread as the one driving the rendering. The engine state owned by the renderer must
 only be touched from that thread
*/
void registerRenderThread();

//! returns 'true' if the calling thread is the one registered as the simulation thread
bool isOnSimulationThread();

//! returns 'true' if the calling thread is the one registered as the render thread
bool isOnRenderThread();

//! asserts in the debug builds that the caller runs on the simulation thread. Compiles to nothing otherwise
inline void assertOnSimulationThread()
{
    assert(isOnSimulationThread());
}

//! asserts in the debug builds that the caller runs on the render thread. Compiles to nothing otherwise
inline void assertOnRenderThread()
{
    assert(isOnRenderThread());
}

}

#endif
