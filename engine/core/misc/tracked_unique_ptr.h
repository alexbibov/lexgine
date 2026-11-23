#ifndef LEXGINE_CORE_MISC_TRACKED_UNIQUE_PTR_H
#define LEXGINE_CORE_MISC_TRACKED_UNIQUE_PTR_H

#include <cassert>
#include <memory>
#include <atomic>

namespace lexgine::core::misc {

// In order to keep the concept of unique ownership this class is deliberately single-threaded,
// i.e. the ownership and all accesses MUST happen from the same thread
template<typename T>
class TrackedUniquePtr
{
public:
   

private:
    
};

}

#endif