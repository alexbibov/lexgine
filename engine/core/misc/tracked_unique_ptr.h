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
private:
    struct service_record;

public:
    class WeakReference
    {
    public:
        WeakReference(T* ptr, service_record* service_record_ptr)
            : m_ptr { ptr }
            , m_service_record_ptr { service_record_ptr }
        {
            assert(m_ptr);
            assert(m_service_record_ptr);
            m_service_record_ptr->ref_count.fetch_add(1, std::memory_order_acq_rel);
        }

        WeakReference(WeakReference const& other)
            : m_ptr { other.m_ptr }
            , m_service_record_ptr { other.m_service_record_ptr }
        {
            if (m_service_record_ptr) 
            {
                assert(m_ptr);
                m_service_record_ptr->ref_count.fetch_add(1, std::memory_order_acq_rel);
            }
        }

        WeakReference(WeakReference&& other)
            : m_ptr { other.m_ptr }
            , m_service_record_ptr { other.m_service_record_ptr }
            , m_is_locked { other.m_is_locked }
        {
            other.m_ptr = nullptr;
            other.m_service_record_ptr = nullptr;
            other.m_is_locked = false;
        }
        
        ~WeakReference()
        {
            if (m_is_locked)
            {
                unlock();
            }
            if (m_service_record_ptr)
            {
                unsigned int old = m_service_record_ptr->ref_count.fetch_sub(1, std::memory_order_acq_rel);
                if (old == 1) 
                {
                    delete m_service_record_ptr;
                }
            }
        }

        T* lock()
        {
            if (m_service_record_ptr) 
            {
                std::unique_lock lock { m_service_record_ptr->lock_mtx };
                m_service_record_ptr->lock_counter.fetch_add(1, std::memory_order_acq_rel);
                m_is_locked = true;
                return m_ptr;
            }
            return nullptr;
        }

        void unlock()
        {
            if (m_service_record_ptr) 
            {
                std::unique_lock lock { m_service_record_ptr->lock_mtx };
                m_service_record_ptr->lock_counter.fetch_sub(1, std::memory_order_acq_rel);
                m_is_locked = false;
            }
        }

    private:
        T* m_ptr { nullptr };
        service_record* m_service_record_ptr { nullptr };
        bool m_is_locked { false };
    };

public:
    TrackedUniquePtr() = default;

    TrackedUniquePtr(T* ptr)
        : m_ptr { ptr }
        , m_service_record { new service_record{ 1, 0, false } }
    {
    }

    TrackedUniquePtr(TrackedUniquePtr&& other)
        : m_ptr{ std::move(other.m_ptr) }
        , m_service_record{ other.m_service_record }
    {
        other.m_service_record = nullptr;
    }

    void reset(T* ptr)
    {
        if (m_service_record_ptr)
        {
            unsigned int old = m_service_record_ptr->ref_count.fetch_sub(1, std::memory_order_acq_rel);
            if (old == 1) 
            {
                delete m_service_record_ptr;
            }
            std::unique_lock lock { m_service_record_ptr->lock_mtx };
            m_lock_cv.wait(
                lock,
                [this] { return m_service_record_ptr->lock_counter.load(std::memory_order_acquire) == 0; });
            m_ptr.reset(ptr)
        }
    }

    ~TrackedUniquePtr()
    {
        reset(nullptr);
    }

private:
    

private:
    struct service_record
    {
        std::atomic_uint ref_count;
        std::atomic_uint lock_counter;
        std::mutex lock_mtx;
    };

private:
    std::unique_ptr<T> m_ptr;
    service_record* m_service_record_ptr{ nullptr };
    std::condition_variable m_lock_cv;
};

}

#endif