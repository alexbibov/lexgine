#ifndef LEXGINE_CORE_STREAMED_CACHE_H
#define LEXGINE_CORE_STREAMED_CACHE_H

#include <cstdint>
#include <memory>
#include <iostream>
#include "data_blob.h"
#include "misc/datetime.h"

namespace lexgine { namespace core {

//! Abstract class implementing main functionality for streamed data cache
class StreamedCache
{
    template<typename Key> friend class CacheEntry;

public:
    StreamedCache(std::ostream& cache_output_stream);

protected:
    void writeData(void* p_source_data_addr, size_t data_size_in_bytes);

private:
    std::ostream& m_write_stream;
};

//! Describes single entry of the cache
template<typename Key>
class CacheEntry
{
public:
    CacheEntry(StreamedCache& cache, Key const& key, DataBlob const& source_data_blob);

    

private:
    StreamedCache& m_cache;
    Key m_key;
    DataBlob const& m_data_blob_to_be_cached;
    misc::DateTime m_date_stamp;
};


struct CacheMess64kbChunk
{
    void* data_addr;
    void* next_chunk_addr;
};


template<typename Key>
inline CacheEntry::CacheEntry(StreamedCache& cache, Key const& key, DataBlob const& source_data_blob):
    m_cache{ cache },
    m_key{ key },
    m_data_blob_to_be_cached{ source_data_blob },
    m_date_stamp{ misc::DateTime::now() }
{

}

}}

#endif
