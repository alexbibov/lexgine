#ifndef LEXGINE_CORE_CONCURRENCY_THREAD_AFFINITY_H
#define LEXGINE_CORE_CONCURRENCY_THREAD_AFFINITY_H

#include <cassert>


namespace lexgine::core::concurrency {

/*! Claims the simulation role for the calling thread. The simulation state must only be touched from a
 thread holding that role
*/
void registerSimulationThread();

/*! Claims the master worker role for the calling thread. The master worker drives the frames: it applies the
 window events, paces the frames, presents them and records the rendering work along with the slave workers
*/
void registerMasterWorkerThread();

/*! Claims the slave worker role for the calling thread. The slave workers record the rendering work of the
 frame the master worker has admitted
*/
void registerSlaveWorkerThread();

//! returns 'true' if the calling thread holds the simulation role
bool isOnSimulationThread();

//! returns 'true' if the calling thread holds the master worker role
bool isOnMasterWorkerThread();

//! returns 'true' if the calling thread holds the slave worker role
bool isOnSlaveWorkerThread();

/*! returns 'true' on any thread recording the rendering work, which is the master worker as well as every
 slave worker
*/
bool isOnRenderThread();

//! asserts in the debug builds that the caller holds the simulation role. Compiles to nothing otherwise
inline void assertOnSimulationThread()
{
    assert(isOnSimulationThread());
}

//! asserts in the debug builds that the caller is the master worker. Compiles to nothing otherwise
inline void assertOnMasterWorkerThread()
{
    assert(isOnMasterWorkerThread());
}

//! asserts in the debug builds that the caller is a slave worker. Compiles to nothing otherwise
inline void assertOnSlaveWorkerThread()
{
    assert(isOnSlaveWorkerThread());
}

//! asserts in the debug builds that the caller records the rendering work. Compiles to nothing otherwise
inline void assertOnRenderThread()
{
    assert(isOnRenderThread());
}

}

#endif
