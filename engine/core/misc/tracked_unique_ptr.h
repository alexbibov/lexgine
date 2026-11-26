#ifndef LEXGINE_CORE_MISC_TRACKED_UNIQUE_PTR_H
#define LEXGINE_CORE_MISC_TRACKED_UNIQUE_PTR_H

#include <cassert>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace lexgine::core::misc {

template<typename T>
class TrackedUniquePtr
{
public:
    TrackedUniquePtr() = default;

    TrackedUniquePtr(T* ptr)
        : m_ptr { ptr }
        , m_service_record { new service_record{ 0, 0, false } }
    {
    }

    TrackedUniquePtr(TrackedUniquePtr&& other)
        : m_ptr{ std::move(other.m_ptr) }
        , m_service_record{ other.m_service_record }
    {
        other.m_service_record = nullptr;
    }

    ~TrackedUniquePtr()
    {
        if (m_service_record_ptr)
        {
            std::unique_lock lock { m_service_record_ptr->lock_mtx };
            m_lock_cv.wait(
                lock,
                [this] { return m_service_record_ptr->lock_counter.load(std::memory_order_acquire) == 0; }
            );
            m_ptr.reset();
            delete m_service_record_ptr;
        }
    }

private:
    struct service_record
    {
        std::atomic_uint generation; 
        std::atomic_uint lock_counter;  // true if at least one weak reference locks the owned object
        std::mutex lock_mtx;
    };

private:
    std::unique_ptr<T> m_ptr;
    service_record* m_service_record_ptr{ nullptr };
    std::condition_variable m_lock_cv;
};

}

#endif