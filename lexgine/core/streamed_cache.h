#ifndef LEXGINE_CORE_STREAMED_CACHE_H
#define LEXGINE_CORE_STREAMED_CACHE_H

#include <cstdint>
#include <memory>
#include <iostream>
#include <vector>
#include "data_blob.h"
#include "misc/datetime.h"

namespace lexgine { namespace core {

//! Describes single entry of the cache
template<typename Key>
class StreamedCacheEntry
{
public:
    using key_type = Key;

public:
    StreamedCacheEntry(Key const& key, DataBlob const& source_data_blob);

private:
    Key m_key;
    DataBlob const& m_data_blob_to_be_cached;
    misc::DateTime m_date_stamp;
};


struct CacheMess64kbChunk
{
    void* data_addr;
    void* next_chunk_addr;
};


// Cache index tree node (the index is implemented by a Red-Black tree)
template<typename Key>
struct StreamedCacheIndexTreeEntry
{
    void* cache_entry_data_address;
    Key cache_entry_key;

    bool node_color;

    StreamedCacheIndexTreeEntry* right_leave;
    StreamedCacheIndexTreeEntry* left_leave;
};


// Cache index structure
template<typename Key>
class StreamedCacheIndex 
{
public:
    using key_type = Key;

public:
    StreamedCacheIndex(StreamedCache<Key>& streamed_cache);

private:
    size_t const m_key_size = Key::size;
    std::vector<StreamedCacheIndexTreeEntry> m_index_tree;
    std::vector<size_t> m_empty_cluster_list;
};


//! Class implementing main functionality for streamed data cache
template<typename Key>
class StreamedCache
{
public:
    using key_type = Key;

public:
    StreamedCache(std::ostream& cache_output_stream, size_t max_cache_size_in_bytes);

    void addEntry(StreamedCacheEntry<Key> const& entry); // ! adds entry into the cache and immediately attempts to write it into the cache stream

    void finalize(); //! writes index data and free cluster look-up table into the end of the cache stream and closes the stream before returning execution control to the caller

    size_t freeSpace() const;    //! returns space yet available to the cache
    
    size_t usedSpace() const;    //! returns space used by the cache so far

private:
    void writeData(void* p_source_data_addr, size_t data_size_in_bytes, size_t stream_write_offset);

private:
    std::ostream& m_write_stream;
    size_t m_max_cache_size;
};



template<typename Key>
inline StreamedCacheEntry::StreamedCacheEntry(Key const& key, DataBlob const& source_data_blob):
    m_key{ key },
    m_data_blob_to_be_cached{ source_data_blob },
    m_date_stamp{ misc::DateTime::now() }
{

}

}}

#endif
