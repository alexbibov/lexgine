#ifndef LEXGINE_CORE_CONCURRENCY_DEFERRED_EVENT_QUEUE_H
#define LEXGINE_CORE_CONCURRENCY_DEFERRED_EVENT_QUEUE_H

#include <cstddef>
#include <mutex>
#include <vector>


namespace lexgine::core::concurrency {

/*! Carries the events recorded on one thread to the thread handling them. The events are recorded where they
 arise, typically from within a window procedure, and are taken in a batch by the thread that handles them, so
 the handling thread never observes an event while it is being recorded.

 The queue is bounded: when the handling thread falls behind, the oldest events are dropped, so the most
 recent state always survives.
*/
template<typename Event>
class DeferredEventQueue final
{
public:
    //! creates a queue holding at most @param capacity events not yet taken
    explicit DeferredEventQueue(size_t capacity)
        : m_capacity{ capacity }
    {
    }

    DeferredEventQueue(DeferredEventQueue const&) = delete;
    DeferredEventQueue& operator=(DeferredEventQueue const&) = delete;

    //! records @param event for the handling thread, dropping the oldest event not yet taken if the queue is full
    void record(Event event)
    {
        std::lock_guard lock{ m_mutex };
        if (m_events.size() >= m_capacity) m_events.erase(m_events.begin());
        m_events.push_back(std::move(event));
    }

    //! removes the events recorded since the previous call and returns them in the order of recording
    std::vector<Event> take()
    {
        std::vector<Event> taken_events;
        {
            std::lock_guard lock{ m_mutex };
            taken_events.swap(m_events);
        }

        return taken_events;
    }

private:
    size_t m_capacity;
    std::mutex m_mutex;
    std::vector<Event> m_events;
};

}

#endif
