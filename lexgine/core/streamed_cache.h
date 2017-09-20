#ifndef LEXGINE_CORE_STREAMED_CACHE_H
#define LEXGINE_CORE_STREAMED_CACHE_H

#include <cstdint>
#include <memory>
#include <iostream>
#include <vector>
#include <mutex>
#include "data_blob.h"
#include "misc/datetime.h"
#include "entity.h"
#include "class_names.h"

namespace lexgine {
namespace core {

//! Describes single entry of the cache
template<typename Key>
class StreamedCacheEntry final
{
    friend class StreamedCache<Key>;

public:
    using key_type = Key;

public:
    StreamedCacheEntry(Key const& key, DataBlob const& source_data_blob);

private:
    Key m_key;
    DataBlob const& m_data_blob_to_be_cached;
    misc::DateTime m_date_stamp;
};


//! Single 64KB cluster of the cache
struct CacheCluster final
{
    char data[65536];
    uint64_t next_cluster_offset;
};


//! Cache index tree node (the index is implemented by a Red-Black tree)
template<typename Key>
struct StreamedCacheIndexTreeEntry final
{
    uint64_t data_offset;
    Key cache_entry_key;

    bool node_color;

    size_t parent_node;
    size_t right_leave;
    size_t left_leave;
};


//! Cache index structure
template<typename Key>
class StreamedCacheIndex final
{
public:
    using key_type = Key;

    void addEntry(std::pair<Key, uint64_t> const& key_offset_pair);    //! adds entry into cache index tree

    void removeEntry(Key const& key);    //! removes entry from cache index tree

    uint64_t getCacheEntryDataOffsetFromKey(Key const& key);    //! retrieves offset of cache entry in the associated stream based on provided key

private:

    size_t bst_insert(std::pair<Key, uint64_t> const& key_offset_pair);    //! standard BST-insertion without RED-BLACK properties check
    void swap_colors(bool& color1, bool& color2);
    void LLcase(size_t p, size_t g);
    void LRcase(size_t p, size_t g);
    void RRcase(size_t p, size_t g);
    void RLcase(size_t p, size_t g);

    size_t const m_key_size = Key::size;
    std::vector<StreamedCacheIndexTreeEntry> m_index_tree;
    std::vector<uint64_t> m_empty_cluster_list;
};


//! Class implementing main functionality for streamed data cache. This class is thread safe.
template<typename Key>
class StreamedCache : public NamedEntity<class_names::StreamedCache>
{
public:
    using key_type = Key;

public:
    StreamedCache(std::ostream& cache_output_stream, size_t max_cache_size_in_bytes);

    void addEntry(StreamedCacheEntry<Key> const& entry); // ! adds entry into the cache and immediately attempts to write it into the cache stream

    void finalize(); //! writes index data and free cluster look-up table into the end of the cache stream and closes the stream before returning execution control to the caller

    size_t freeSpace() const;    //! returns space yet available to the cache

    size_t usedSpace() const;    //! returns space used by the cache so far

    StreamedCacheEntry retrieveEntry(Key const& key) const;    //! retrieves an entry from the cache based on its key

    void removeEntry(Key const& key) const;    //! removes entry from the cache (and immediately from its associated stream) given its key

private:
    void writeData(void* p_source_data_addr, size_t data_size_in_bytes, size_t stream_write_offset);


private:
    std::ostream& m_write_stream;
    size_t m_max_cache_size;
};



template<typename Key>
inline StreamedCacheEntry<Key>::StreamedCacheEntry(Key const& key, DataBlob const& source_data_blob) :
    m_key{ key },
    m_data_blob_to_be_cached{ source_data_blob },
    m_date_stamp{ misc::DateTime::now() }
{

}


template<typename Key>
inline void core::StreamedCacheIndex<Key>::addEntry(std::pair<Key, uint64_t> const& key_offset_pair)
{
    if (m_index_tree.size() == 0)
    {
        StreamedCacheIndexTreeEntry root_entry;

        root_entry.data_offset = key_offset_pair.second;
        root_entry.cache_entry_key = key_offset_pair.first;
        root_entry.node_color = false;    // root is always BLACK
        root_entry.parent_node = 0;    // 0 is index of the tree buffer where the root of the tree always resides. But no leave ever has root as child, so use 0 to encode leave nodes
        root_entry.right_leave = 0;
        root_entry.left_leave = 0;

        m_index_tree.push_back(root_entry);
    }
    else
    {
        size_t parent_idx = bst_insert(key_offset_pair);
        size_t current_idx = m_index_tree.size() - 1;

        bool parent_color = m_index_tree[parent_idx];
        if (parent_color)    // parent_color=true means it's RED
        {
            // in this case the RED-BLACK structure of the tree is corrupted and needs to be recovered

            size_t grandparent_idx = m_index_tree[parent_idx].parent_node;
            bool T1 = m_index_tree[grandparent_idx].left_leave == parent_idx;    // does parent reside in the left sub-tree?
            size_t uncle_idx = T1 ? m_index_tree[grandparent_idx].right_leave : m_index_tree[grandparent_idx].left_leave;

            bool root_reached{ false };
            while (!root_reached && uncle_idx && m_index_tree[uncle_idx].node_color)
            {
                // uncle is RED case
                m_index_tree[parent_idx].node_color = false;
                m_index_tree[uncle_idx].node_color = false;
                m_index_tree[grandparent_idx].node_color = grandparent_idx != 0;
                root_reached = grandparent_idx == 0;

                current_idx = grandparent_idx;
                parent_idx = m_index_tree[current_idx].parent_node;
                grandparent_idx = m_index_tree[parent_idx].parent_node;
                T1 = m_index_tree[grandparent_idx].left_leave == parent_idx;
                uncle_idx = T1 ? m_index_tree[grandparent_idx].right_leave : m_index_tree[grandparent_idx].left_leave;
            }

            if (!root_reached)
            {
                // uncle is BLACK cases

                bool T2 = m_index_tree[parent_idx].left_leave == current_idx;    // does new node reside in the left sub-tree?

                if (T1 && T2)
                {
                    // Left-Left case
                    LLcase(parent_idx, grandparent_idx);
                }
                else if (T1 && !T2)
                {
                    // Left-Right case
                    LRcase(parent_idx, grandparent_idx);
                }
                else if (!T1 && T2)
                {
                    // Right-Left case
                    RLcase(parent_idx, grandparent_idx);
                }
                else
                {
                    // Right-Right case
                    RRcase(parent_idx, grandparent_idx);
                }
            }
        }
    }
}

template<typename Key>
inline size_t core::StreamedCacheIndex<Key>::bst_insert(std::pair<Key, uint64_t> const& key_offset_pair)
{
    size_t const target_index = m_index_tree.size();

    size_t insertion_node_idx;
    bool is_insertion_subtree_left;
    {
        size_t search_idx{ 0 };
        do
        {
            insertion_node_idx = search_idx;
            search_idx = (is_insertion_subtree_left = new_entry.cache_entry_key < m_index_tree[search_idx].cache_entry_key)
                ? m_index_tree[search_idx].left_leave
                : m_index_tree[search_idx].right_leave;
        } while (search_idx);
    }

    // setup connection to the new node
    if (is_insertion_subtree_left) m_index_tree[insertion_node_idx].left_leave = target_index;
    else m_index_tree[insertion_node_idx].right_leave = target_index;

    // finally, physically insert new node into the tree buffer
    StreamedCacheIndexTreeEntry new_entry;
    new_entry.data_offset = key_offset_pair.second;
    new_entry.cache_entry_key = key_offset_pair.first;
    new_entry.node_color = true;    // newly inserted nodes are RED
    new_entry.parent_node = insertion_node_idx;
    new_entry.left_leave = 0;
    new_entry.right_leave = 0;
    m_index_tree.push_back(new_entry);

    return insertion_node_idx;
}

template<typename Key>
inline void core::StreamedCacheIndex<Key>::swap_colors(bool& color1, bool& color2)
{
    color1 = color1^color2;
    color2 = color2^color1;
    color1 = color1^color2;
}

template<typename Key>
inline void core::StreamedCacheIndex<Key>::LLcase(size_t p, size_t g)
{
    m_index_tree[g].left_leave = m_index_tree[p].right_leave;
    m_index_tree[p].right_leave = g;
    swap_colors(m_index_tree[p].node_color, m_index_tree[g].node_color);
}

template<typename Key>
inline void StreamedCacheIndex<Key>::LRcase(size_t p, size_t g)
{
    size_t current_idx = m_index_tree[p].right_leave;
    m_index_tree[p].right_leave = m_index_tree[current_idx].left_leave;
    m_index_tree[current_idx].left_leave = p;
    LLcase(current_idx, g);
}

template<typename Key>
inline void StreamedCacheIndex<Key>::RRcase(size_t p, size_t g)
{
    m_index_tree[g].right_leave = m_index_tree[p].left_leave;
    m_index_tree[p].left_leave = g;
    swap_colors(m_index_tree[p].node_color, m_index_tree[g].node_color);
}

template<typename Key>
inline void StreamedCacheIndex<Key>::RLcase(size_t p, size_t g)
{
    size_t current_index = m_index_tree[p].left_leave;
    m_index_tree[p].left_leave = m_index_tree[current_index].right_leave;
    m_index_tree[current_index].right_leave = p;
    RRcase(x, g);
}


}}

#endif
