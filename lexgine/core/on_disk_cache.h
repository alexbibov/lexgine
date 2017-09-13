#ifndef LEXGINE_CORE_ON_DISK_CACHE_H
#define LEXGINE_CORE_ON_DISK_CACHE_H

#include <cstdint>

namespace lexgine { namespace core {

//! Abstract class implementing main functionality for on disk caches
class OnDiskCache
{
public:
    
    //! Describes single entry of the cache
    template<typename Key>
    class CacheEntry
    {
    public:
        CacheEntry(Key const& key, void* source_data, size_t source_data_size);

    private:
        size_t m_entry_size;
        
        
    
    };

public:

    

private:

    

};

}

}

#endif
