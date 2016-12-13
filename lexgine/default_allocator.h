#ifndef LEXGINE_CORE_DEFAULT_ALLOCATOR_H


namespace lexgine {namespace core {

//! Implements default allocator for objects of type T (type T must implement default constructor)
template<typename T>
class DefaultAllocator
{
public:
    //! Allocates new object of type T from the heap memory using default constructor
    T* allocate()
    {
        return new T{};
    }

    //! Removes the object instance at the address provided from the heap and guarantees that the object gets properly destructed
    void free(T* p_instance)
    {
        delete p_instance;
    }
};

}}

#define LEXGINE_CORE_DEFAULT_ALLOCATOR_H
#endif
