#ifndef LEXGINE_CORE_MISC_STATIC_VECTOR_H
#define LEXGINE_CORE_MISC_STATIC_VECTOR_H

#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <cassert>
#include <iterator>

namespace lexgine::core::misc {

template<typename T, size_t max_size>
class StaticVector
{
public:
    class StaticVectorIterator
    {
        friend class StaticVector<T, max_size>;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = size_t;
        using pointer = T*;
        using reference = T&;

    public:

        // required by output iterator canonical implementation

        StaticVectorIterator(StaticVectorIterator const& other);
        T& operator*();
        StaticVectorIterator& operator++();
        StaticVectorIterator operator++(int);

        // required by input iterator canonical implementation
        T const& operator*() const;
        T const* operator->() const;
        bool operator == (StaticVectorIterator const& other) const;
        bool operator != (StaticVectorIterator const& other) const;

        // required by forward iterator canonical implementation
        T* operator->();
        StaticVectorIterator();
        StaticVectorIterator& operator=(StaticVectorIterator const& other);

        // required by bidirectional iterator canonical implementation
        StaticVectorIterator& operator--();
        StaticVectorIterator operator--(int);

        // required by random-access iterator canonical implementation
        T& operator[](size_t index);
        T const& operator[](size_t index) const;
        StaticVectorIterator& operator+=(size_t n);
        StaticVectorIterator& operator-=(size_t n);
        StaticVectorIterator operator+(size_t n) const;
        StaticVectorIterator operator-(size_t n) const;
        size_t operator-(StaticVectorIterator const& other) const;
        bool operator<(StaticVectorIterator const& other) const;
        bool operator>(StaticVectorIterator const& other) const;
        bool operator<=(StaticVectorIterator const& other) const;
        bool operator>=(StaticVectorIterator const& other) const;

    private:
        StaticVectorIterator(char* addr, size_t const& vector_size);

    private:
        T* m_start;
        T* m_current;
        size_t const* m_referred_vector_size_ptr;
    };

    using value_type = T;

    using iterator = StaticVectorIterator;
    using const_iterator = StaticVectorIterator const;

    StaticVector();
    StaticVector(size_t num_elements, T const& val);
    StaticVector(size_t num_elements);
    StaticVector(std::initializer_list<T> elements);
    StaticVector(StaticVector const& other);
    StaticVector(StaticVector&& other);

    StaticVector& operator=(StaticVector const& other);
    StaticVector& operator=(StaticVector&& other);

    virtual ~StaticVector();

    T& operator[](size_t index);
    T const& operator[](size_t index) const;

    void push_back(T const& new_element);

    template<typename ...Args>
    void emplace_back(Args... args);

    T pop_back();

    void resize(size_t new_size);

    size_t size() const;
    static constexpr size_t capacity() { return m_capacity / sizeof(T); }

    StaticVectorIterator find(T const& value) const;

    iterator begin() { return StaticVectorIterator{ m_data, m_size }; }
    iterator end() { return StaticVectorIterator{ m_p_end, m_size }; }

    const_iterator cbegin() const { return const_cast<StaticVector*>(this)->begin(); }
    const_iterator cend() const { return const_cast<StaticVector*>(this)->end(); }

    const_iterator begin() const { return cbegin(); }
    const_iterator end() const { return cend(); }

    T* data() { return reinterpret_cast<T*>(m_data); }
    T const* data() const { return reinterpret_cast<T*>(m_data); }

private:
    template<typename = typename std::enable_if<std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value>::type>
    static void destruct_element(T* p_e) { p_e->~T(); }

    template<typename = typename std::enable_if<!std::is_destructible<T>::value || std::is_trivially_destructible<T>::value>::type>
    static void destruct_element(T* p_e, int = 0) {}

private:
    static constexpr size_t m_capacity = sizeof(T)*max_size;
    char m_data[m_capacity];
    char* m_p_end;
    size_t m_size;

#ifdef _DEBUG
    T* m_elements;
#endif
};

template<typename T, size_t max_size>
inline StaticVector<T, max_size>::StaticVector() :
    m_data{ 0 },
    m_p_end{ m_data },
    m_size{ 0U }
{
#ifdef _DEBUG
    m_elements = reinterpret_cast<T*>(m_data);
#endif

}

template<typename T, size_t max_size>
inline StaticVector<T, max_size>::StaticVector(size_t num_elements, T const& val) :
    m_data{ 0 },
    m_p_end{ m_data },
    m_size{ 0U }
{
#ifdef _DEBUG
    m_elements = reinterpret_cast<T*>(m_data);
#endif

    while (m_size < num_elements) push_back(val);
}

template<typename T, size_t max_size>
inline StaticVector<T, max_size>::StaticVector(size_t num_elements) :
    m_data{ 0 },
    m_p_end{ m_data },
    m_size{ 0U }
{
#ifdef _DEBUG
    m_elements = reinterpret_cast<T*>(m_data);
#endif

    while (m_size < num_elements) push_back(T{});
}

template<typename T, size_t max_size>
inline StaticVector<T, max_size>::StaticVector(std::initializer_list<T> elements) :
    m_data{ 0 },
    m_p_end{ m_data },
    m_size{ 0U }
{
#ifdef _DEBUG
    m_elements = reinterpret_cast<T*>(m_data);
#endif

    for (auto const& e : elements)
    {
        if (m_size < max_size) push_back(e);
        else break;
    }
}

template<typename T, size_t max_size>
inline StaticVector<T, max_size>::StaticVector(StaticVector const& other):
    m_data{ 0 },
    m_p_end{ m_data },
    m_size{ 0U }
{
#ifdef _DEBUG
    m_elements = reinterpret_cast<T*>(m_data);
#endif

    for (auto& e : other)
    {
        push_back(e);
    }
}

template<typename T, size_t max_size>
inline StaticVector<T, max_size>::StaticVector(StaticVector&& other):
    m_data{ 0 },
    m_p_end{ m_data },
    m_size{ 0U }
{
#ifdef _DEBUG
    m_elements = reinterpret_cast<T*>(m_data);
#endif

    for (auto& e : other)
    {
        emplace_back(std::move(e));
    }
}

template<typename T, size_t max_size>
inline StaticVector<T, max_size>& StaticVector<T, max_size>::operator=(StaticVector const& other)
{
    if (this == &other)
        return *this;

    while (m_size > other.m_size) pop_back();

    for (size_t i = 0; i < m_size; ++i)
        (*this)[i] = other[i];

    for (size_t i = m_size; i < other.m_size; ++i)
        push_back(other[i]);

    return *this;
}

template<typename T, size_t max_size>
inline StaticVector<T, max_size>& StaticVector<T, max_size>::operator=(StaticVector&& other)
{
    if (this == &other)
        return this;

    while (m_size > other.m_size) pop_back();

    for (size_t i = 0; i < m_size; ++i)
        (*this)[i] = std::move(other[i]);

    for (size_t i = m_size; i < other.m_size; ++i)
        emplace_back(std::move(other[i]));

    return *this;
}

template<typename T, size_t max_size>
inline StaticVector<T, max_size>::~StaticVector()
{
    while (m_size > 0) pop_back();
}

template<typename T, size_t max_size>
inline T& StaticVector<T, max_size>::operator[](size_t index)
{
    assert(index < m_size);
    return *reinterpret_cast<T*>(&m_data[sizeof(T)*index]);
}

template<typename T, size_t max_size>
inline T const& StaticVector<T, max_size>::operator[](size_t index) const
{
    return const_cast<StaticVector<T, max_size>*>(this)->operator[](index);
}

template<typename T, size_t max_size>
inline void StaticVector<T, max_size>::push_back(T const& new_element)
{
    assert(m_size < max_size);
    new (m_p_end) T{ new_element };

    m_p_end += sizeof(T);
    ++m_size;
}

template<typename T, size_t max_size>
template<typename ...Args>
inline void StaticVector<T, max_size>::emplace_back(Args ...args)
{
    assert(m_size < max_size);
    new (m_p_end) T{ std::forward<Args>(args)... };

    m_p_end += sizeof(T);
    ++m_size;
}

template<typename T, size_t max_size>
inline T StaticVector<T, max_size>::pop_back()
{
    m_p_end -= sizeof(T);
    T* e_addr = reinterpret_cast<T*>(m_p_end);
    T rv{ std::move(*e_addr) };
    destruct_element(e_addr);
    --m_size;
    return rv;
}

template<typename T, size_t max_size>
inline void StaticVector<T, max_size>::resize(size_t new_size)
{
    while (m_size < new_size)
        push_back(T{});

    while (m_size > new_size)
        pop_back();
}

template<typename T, size_t max_size>
inline size_t StaticVector<T, max_size>::size() const
{
    return m_size;
}

template<typename T, size_t max_size>
inline typename StaticVector<T, max_size>::StaticVectorIterator StaticVector<T, max_size>::find(T const& value) const
{
    for (char* addr = m_data; addr < m_data + sizeof(T)*m_size; addr += sizeof(T))
    {
        if (*reinterpret_cast<T*>(addr) == value)
            return StaticVectorIterator{ addr, m_size };
    }

    return end();
}


// required by random-access iterator canonical implementation
template<typename T, size_t max_size>
inline typename StaticVector<T, max_size>::StaticVectorIterator operator+(size_t n, 
    typename StaticVector<T, max_size>::iterator const& other)
{
    return other + n;
}

template<typename T, size_t max_size>
inline StaticVector<T, max_size>::StaticVectorIterator::StaticVectorIterator(StaticVectorIterator const& other) :
    m_start{ other.m_start },
    m_current{ other.m_current },
    m_referred_vector_size_ptr{ other.m_referred_vector_size_ptr}
{

}

template<typename T, size_t max_size>
inline T& StaticVector<T, max_size>::StaticVectorIterator::operator*()
{
    assert(m_current < *m_referred_vector_size_ptr + m_start);
    return *m_current;
}

template<typename T, size_t max_size>
inline typename StaticVector<T, max_size>::StaticVectorIterator& StaticVector<T, max_size>::StaticVectorIterator::operator++()
{
    ++m_current;
    assert(m_current < *m_referred_vector_size_ptr + m_start);
    return *this;
}

template<typename T, size_t max_size>
inline typename StaticVector<T, max_size>::StaticVectorIterator StaticVector<T, max_size>::StaticVectorIterator::operator++(int)
{
    StaticVectorIterator rv{ *this };
    ++m_current;
    assert(m_current < *m_referred_vector_size_ptr + m_start);
    return rv;
}

template<typename T, size_t max_size>
inline T const& StaticVector<T, max_size>::StaticVectorIterator::operator*() const
{
    assert(m_current < *m_referred_vector_size_ptr + m_start);
    return *m_current;
}

template<typename T, size_t max_size>
inline T const* StaticVector<T, max_size>::StaticVectorIterator::operator->() const
{
    assert(m_current < *m_referred_vector_size_ptr + m_start);
    return m_current;
}

template<typename T, size_t max_size>
inline bool StaticVector<T, max_size>::StaticVectorIterator::operator==(StaticVectorIterator const& other) const
{
    return m_current == other.m_current;
}

template<typename T, size_t max_size>
inline bool StaticVector<T, max_size>::StaticVectorIterator::operator!=(StaticVectorIterator const& other) const
{
    return m_current != other.m_current;
}

template<typename T, size_t max_size>
inline T* StaticVector<T, max_size>::StaticVectorIterator::operator->()
{
    assert(m_current < *m_referred_vector_size_ptr + m_start);
    return m_current;
}

template<typename T, size_t max_size>
inline StaticVector<T, max_size>::StaticVectorIterator::StaticVectorIterator() :
    m_start{ nullptr },
    m_current{ nullptr },
    m_referred_vector_size_ptr{ nullptr }
{
}

template<typename T, size_t max_size>
inline typename StaticVector<T, max_size>::StaticVectorIterator& StaticVector<T, max_size>::StaticVectorIterator::operator=(StaticVectorIterator const& other)
{
    m_start = other.m_start;
    m_current = other.m_current;
    m_referred_vector_size_ptr = other.m_referred_vector_size_ptr;
    return *this;
}

template<typename T, size_t max_size>
inline typename StaticVector<T, max_size>::StaticVectorIterator& StaticVector<T, max_size>::StaticVectorIterator::operator--()
{
    --m_current;
    assert(m_current >= m_start);
    return *this;
}

template<typename T, size_t max_size>
inline typename StaticVector<T, max_size>::StaticVectorIterator StaticVector<T, max_size>::StaticVectorIterator::operator--(int)
{
    StaticVectorIterator rv{ *this };
    --m_current;
    assert(m_current >= m_start);
    return rv;
}

template<typename T, size_t max_size>
inline T& StaticVector<T, max_size>::StaticVectorIterator::operator[](size_t index)
{
    T* element = m_current + index;
    assert(element < *m_referred_vector_size_ptr + m_start);
    return *element;
}

template<typename T, size_t max_size>
inline T const& StaticVector<T, max_size>::StaticVectorIterator::operator[](size_t index) const
{
    return (*const_cast<StaticVectorIterator*>(this))[index];
}

template<typename T, size_t max_size>
inline typename StaticVector<T, max_size>::StaticVectorIterator& StaticVector<T, max_size>::StaticVectorIterator::operator+=(size_t n)
{
    m_current += n;
    assert(m_current < *m_referred_vector_size_ptr + m_start);
    return *this;
}

template<typename T, size_t max_size>
inline typename StaticVector<T, max_size>::StaticVectorIterator& StaticVector<T, max_size>::StaticVectorIterator::operator-=(size_t n)
{
    assert(m_current >= m_start + n);
    m_current -= n;
    return *this;
}

template<typename T, size_t max_size>
inline typename StaticVector<T, max_size>::StaticVectorIterator StaticVector<T, max_size>::StaticVectorIterator::operator+(size_t n) const
{
    T* ptr = m_current + n;
    assert(ptr < *m_referred_vector_size_ptr + m_start);
    return StaticVectorIterator{ reinterpret_cast<char*>(ptr), *m_referred_vector_size_ptr };
}

template<typename T, size_t max_size>
inline typename StaticVector<T, max_size>::StaticVectorIterator StaticVector<T, max_size>::StaticVectorIterator::operator-(size_t n) const
{
    assert(m_current >= m_start + n);
    T* ptr = m_current - n;
    return StaticVectorIterator{ reinterpret_cast<char*>(ptr), *m_referred_vector_size_ptr };
}

template<typename T, size_t max_size>
inline size_t StaticVector<T, max_size>::StaticVectorIterator::operator-(StaticVectorIterator const& other) const
{
    assert(m_current >= other.m_current);
    return static_cast<size_t>(m_current - other.m_current);
}

template<typename T, size_t max_size>
inline bool StaticVector<T, max_size>::StaticVectorIterator::operator<(StaticVectorIterator const& other) const
{
    return m_current < other.m_current;
}

template<typename T, size_t max_size>
inline bool StaticVector<T, max_size>::StaticVectorIterator::operator>(StaticVectorIterator const& other) const
{
    return m_current > other.m_current;
}

template<typename T, size_t max_size>
inline bool StaticVector<T, max_size>::StaticVectorIterator::operator<=(StaticVectorIterator const& other) const
{
    return m_current <= other.m_current;
}

template<typename T, size_t max_size>
inline bool StaticVector<T, max_size>::StaticVectorIterator::operator>=(StaticVectorIterator const& other) const
{
    return m_current >= other.m_current;
}

template<typename T, size_t max_size>
inline StaticVector<T, max_size>::StaticVectorIterator::StaticVectorIterator(char* addr, size_t const& vector_size) :
    m_start{ reinterpret_cast<T*>(addr) },
    m_current{ m_start },
    m_referred_vector_size_ptr{ &vector_size }
{

}

}

#endif
