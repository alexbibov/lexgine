#ifndef LEXGINE_CORE_STREAMED_CACHE_H
#define LEXGINE_CORE_STREAMED_CACHE_H

#include <cstdint>
#include <memory>
#include <iostream>
#include <vector>
#include <mutex>
#include <iterator>
#include "data_blob.h"
#include "misc/datetime.h"
#include "entity.h"
#include "class_names.h"
#include "misc/optional.h"

namespace lexgine {
namespace core {

//! Describes single entry of the cache
template<typename Key, size_t cluster_size>
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


//! Cache index tree node (the index is implemented by a Red-Black tree)
template<typename Key>
struct StreamedCacheIndexTreeEntry final
{
    enum class inheritance_category : unsigned char
    {
        root = 0,
        left_child = 1,
        right_child = 2
    };

    bool to_be_deleted = false;

    uint64_t data_offset;
    Key cache_entry_key;

    unsigned char node_color;
    inheritance_category inheritance;    //!< 0 is root; 1 is left child; 2 is right child

    size_t parent_node;
    size_t right_leaf;
    size_t left_leaf;

    size_t const key_serialized_size = Key::serialized_size;
    size_t const serialized_size = 32    //!< left and right leafs, parent node index, and the data offset
        + key_serialized_size    //!< size of serialized key
        + 1;    //!< node color (2 low order bits) + inheritance category (2 next bits) + is subject for deletion at some point (1 bit) + 3 currently unused bits   

    void prepare_serialization_blob(void* p_blob_memory);    //! serializes entry to memory provided (and owned) by the caller
    void deserialize_from_blob(void* p_blob_memory);    //! fills in the entry data based on serialized blob provided by the caller
};


//! Cache index structure
template<typename Key>
class StreamedCacheIndex final
{
    friend class StreamedCache<Key>;


public:

    class StreamedCacheIndexIterator : public std::iterator<std::bidirectional_iterator_tag, StreamedCacheIndexTreeEntry<Key>>
    {
        friend class StreamedCacheIndex;

    public:
        // required by output iterator standard behavior

        StreamedCacheIndexIterator(StreamedCacheIndexIterator const& other) = default;
        StreamedCacheIndexIterator& operator++();
        StreamedCacheIndexIterator operator++(int);
        StreamedCacheIndexTreeEntry<Key>& operator*();


        // required by input iterator standard behavior

        StreamedCacheIndexTreeEntry<Key> const& operator*() const;
        StreamedCacheIndexTreeEntry<Key> const* operator->() const;
        bool operator==(StreamedCacheIndexIterator const& other) const;
        bool operator!=(StreamedCacheIndexIterator const& other) const;


        // required by forward iterator standard behavior

        StreamedCacheIndexIterator() = default;
        StreamedCacheIndexTreeEntry<Key>* operator->();
        StreamedCacheIndexIterator& operator=(StreamedCacheIndexIterator const& other) = default;


        // required by bidirectional iterator standard behavior

        StreamedCacheIndexIterator& operator--();
        StreamedCacheIndexIterator operator--(int);

    private:
        bool m_is_at_beginning = false;
        bool m_has_reached_end = true;
        size_t m_current_index = 0U;
        vector<StreamedCacheIndexTreeEntry<Key>>* m_p_target_index_tree = nullptr;
    };

public:

    using key_type = Key;
    using iterator = StreamedCacheIndexIterator;
    using const_iterator = StreamedCacheIndexIterator const;

public:

    misc::Optional<uint64_t> getCacheEntryDataOffsetFromKey(Key const& key) const;    //! retrieves offset of cache entry in the associated stream based on provided key

    size_t getCurrentRedundancy() const;    //! returns current redundancy of the index tree buffer represented in bytes

    size_t getMaxAllowedRedundancy() const;    //! returns maximal allowed redundancy of the tree buffer represented in bytes

    /*! Sets maximal redundancy allowed for the index tree buffer represented in bytes. Note that the real maximal allowed redundancy applied to the index tree
     buffer may be slightly less then the value provided to this function as the latter gets aligned to the size of certain internal structure representing
     single entry of the index tree. Call getMaxAllowedRedundancy() to get factual redundancy value applied to the index tree buffer
    */
    void setMaxAllowedRedundancy(size_t max_redundancy_in_bytes);

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

private:
    void add_entry(std::pair<Key, uint64_t> const& key_offset_pair);    //! adds entry into cache index tree
    bool remove_entry(Key const& key);    //! removes entry from cache index tree


    size_t bst_insert(std::pair<Key, uint64_t> const& key_offset_pair);    //! standard BST-insertion without RED-BLACK properties check
    static void swap_colors(unsigned char& color1, unsigned char& color2);
    static uint32_t locate_bin(size_t n, std::vector<size_t> const& bins_in_accending_order);

    void right_rotate(size_t a, size_t b);
    void left_rotate(size_t a, size_t b);

    std::tuple<size_t, size_t, bool> bst_delete(Key const& key);    //! standard BST deletion based on provided key

    std::pair<size_t, bool> bst_search(Key const& key);    //! retrieves address of the node having the given key. The second element of returned pair is 'true' if the node has been found and 'false' otherwise.

    void rebuild_index();    //! builds new index tree buffer cleaned up from unused entries

private:

    size_t const m_key_size = Key::size;
    std::vector<StreamedCacheIndexTreeEntry<Key>> m_index_tree;
    std::vector<uint64_t> m_empty_cluster_list;
    size_t m_current_index_redundant_growth_pressure = 0U;
    size_t m_max_index_redundant_growth_pressure = 1000U;    //!< maximal allowed amount of unused entries in the index tree buffer, after which the buffer is rebuilt
};


//! Class implementing main functionality for streamed data cache. This class is thread safe.
template<typename Key, size_t cluster_size = 16384U>
class StreamedCache : public NamedEntity<class_names::StreamedCache>
{
public:
    using key_type = Key;

public:
    StreamedCache(std::iostream& cache_io_stream, size_t max_cache_size_in_bytes);    //! initializes new cache
    StreamedCache(std::iostream& cache_io_stream);    //! opens IO stream containing existing cache
    virtual ~StreamedCache();

    void addEntry(StreamedCacheEntry<Key, cluster_size> const& entry);    //! adds entry into the cache and immediately attempts to write it into the cache stream

    void finalize(); //! writes index data and free cluster look-up table into the end of the cache stream and closes the stream before returning execution control to the caller

    size_t freeSpace() const;    //! returns space yet available to the cache

    size_t usedSpace() const;    //! returns space used by the cache so far

    StreamedCacheEntry<Key, cluster_size> retrieveEntry(Key const& key) const;    //! retrieves an entry from the cache based on its key

    void removeEntry(Key const& key) const;    //! removes entry from the cache (and immediately from its associated stream) given its key

private:
    void rebuild_empty_cluster_table();
    void serialize_entry(StreamedCacheEntry const& entry);
    StreamedCacheEntry deserialize_entry();
    static void pack_date_stamp(misc::DateTime const& date_stamp, char packed_date_stamp[13]);
    static misc::DateTime unpack_date_stamp(char packed_date_stamp[13]);

private:
    using empty_cluster_reference = std::pair<size_t, size_t>;

private:
    std::iostream& m_cache_stream;
    size_t m_max_cache_size;
    size_t m_current_cache_size;
    StreamedCacheIndex<Key> m_index;
    std::vector<empty_cluster_reference> m_empty_cluster_table;
};



template<typename Key, size_t cluster_size>
inline StreamedCacheEntry<Key, cluster_size>::StreamedCacheEntry(Key const& key, DataBlob const& source_data_blob) :
    m_key{ key },
    m_data_blob_to_be_cached{ source_data_blob },
    m_date_stamp{ misc::DateTime::now() }
{

}

template<typename Key, size_t cluster_size>
inline void StreamedCacheEntry<Key, cluster_size>::pack_date_stamp(misc::DateTime const& date_stamp, char packed_date_stamp[13])
{
    *static_cast<uint16_t*>(packed_date_stamp) = date_stamp.year();    // 16-bit storage for year
    uint8_t month = date_stamp.month();
    uint8_t day = date_stamp.day();
    uint8_t hour = date_stamp.hour();
    uint8_t minute = date_stamp.minute();
    double second = date_stamp.second();

    packed_date_stamp[2] = static_cast<uint8_t>(month);    // 4-bit storage for month
    packed_date_stamp[2] |= day << 4;    // 5-bit storage for day
    packed_date_stamp[3] = day >> 4;
    packed_date_stamp[3] |= hour << 1;    // 5-bit storage for hour
    packed_date_stamp[3] |= minute << 6;    // 6-bit storage for minute
    packed_date_stamp[4] |= minute >> 2;    // 4-bits reserved for future use (time zones?)

    *static_cast<double*>(packed_date_stamp + 5) = second;    // 64-bit storage for high-precision second
}

template<typename Key, size_t cluster_size>
inline misc::DateTime StreamedCacheEntry<Key, cluster_size>::unpack_date_stamp(char packed_date_stamp[13])
{
    uint16_t year = *static_cast<uint16_t*>(packed_date_stamp);
    uint8_t month = packed_date_stamp[2] & 0xF;
    uint8_t day = packed_date_stamp[2] >> 4; day |= (packed_date_stamp[3] & 0x1) << 4;
    uint8_t hour = (packed_date_stamp[3] & 0x3E) >> 1;
    uint8_t minute = packed_date_stamp[3] >> 6; minute |= packed_date_stamp[4] << 2;
    double second = *static_cast<double*>(packed_date_stamp + 5);

    return misc::DateTime{ year, month, day, hour, minute, second };
}

template<typename Key>
inline misc::Optional<uint64_t> StreamedCacheIndex<Key>::getCacheEntryDataOffsetFromKey(Key const& key) const
{
    std::pair<size_t, bool> search_result = bst_search(key);
    if (search_result.second)
        return misc::Optional<uint64_t>{ m_index_tree[search_result.first].data_offset };

    return misc::Optional<uint64_t>{};
}

template<typename Key>
inline size_t StreamedCacheIndex<Key>::getCurrentRedundancy() const
{
    return m_current_index_redundant_growth_pressure * StreamedCacheIndexTreeEntry<Key>::serialized_size;
}

template<typename Key>
inline size_t StreamedCacheIndex<Key>::getMaxAllowedRedundancy() const
{
    return m_max_index_redundant_growth_pressure * StreamedCacheIndexTreeEntry<Key>::serialized_size;
}

template<typename Key>
inline void StreamedCacheIndex<Key>::setMaxAllowedRedundancy(size_t max_redundancy_in_bytes)
{
    m_max_index_redundant_growth_pressure = max_redundancy_in_bytes / StreamedCacheIndexTreeEntry<Key>::serialized_size;
}

template<typename Key>
inline iterator StreamedCacheIndex<Key>::begin()
{
    iterator rv{};
    rv.m_is_at_beginning = true;
    rv.m_has_reached_end = false;
    rv.m_p_target_index_tree = &m_index_tree;
    return rv;
}

template<typename Key>
inline iterator StreamedCacheIndex<Key>::end()
{
    iterator rv{};
    rv.m_p_target_index_tree = &m_index_tree;
    return rv;
}

template<typename Key>
inline const_iterator StreamedCacheIndex<Key>::begin() const
{
    return const_cast<StreamedCacheIndex<Key>*>(this)->begin();
}

template<typename Key>
inline const_iterator StreamedCacheIndex<Key>::end() const
{
    return const_cast<StreamedCacheIndex<Key>*>(this)->end();
}

template<typename Key>
inline const_iterator StreamedCacheIndex<Key>::cbegin() const
{
    return begin();
}

template<typename Key>
inline const_iterator StreamedCacheIndex<Key>::cend() const
{
    return end();
}

template<typename Key>
inline void core::StreamedCacheIndex<Key>::add_entry(std::pair<Key, uint64_t> const& key_offset_pair)
{
    if (m_index_tree[0].to_be_deleted)
    {
        // we are adding root
        m_index_tree.clear();    // ensure that the index buffer is empty, if there were redundant previously deleted nodes, it's perfect time the remove them
        m_current_index_redundant_growth_pressure = 0U;

        StreamedCacheIndexTreeEntry<Key> root_entry;

        root_entry.data_offset = key_offset_pair.second;
        root_entry.cache_entry_key = key_offset_pair.first;
        root_entry.node_color = 1;    // root is always BLACK
        root_entry.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::root;
        root_entry.parent_node = 0;    // 0 is index of the tree buffer where the root of the tree always resides. But no leave ever has root as child, so use 0 to encode leave nodes
        root_entry.right_leaf = 0;
        root_entry.left_leaf = 0;

        m_index_tree.push_back(root_entry);
    }
    else
    {
        size_t parent_idx = bst_insert(key_offset_pair);
        size_t current_idx = m_index_tree.size() - 1;
        StreamedCacheIndexTreeEntry<Key>& parent_of_inserted_node = m_index_tree[parent_idx];
        StreamedCacheIndexTreeEntry<Key>& inserted_node = &m_index_tree[current_idx];

        size_t grandparent_idx = parent_of_inserted_node.parent_node;
        StreamedCacheIndexTreeEntry<Key>& grandparent_of_inserted_node = m_index_tree[grandparent_idx];

        if (parent_of_inserted_node.node_color == 0)
        {
            // parent is RED:
            // in this case the RED-BLACK structure of the tree is corrupted and needs to be recovered


            bool T1 = parent_of_inserted_node.inheritance
                == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;
            size_t uncle_idx = T1 ? grandparent_of_inserted_node.right_leaf : grandparent_of_inserted_node.left_leaf;

            bool root_reached{ false };
            while (!root_reached && uncle_idx && m_index_tree[uncle_idx].node_color == 0)
            {
                // uncle is RED case

                root_reached = grandparent_idx == 0;

                StreamedCacheIndexTreeEntry<Key>& parent_of_current_node = m_index_tree[parent_idx];
                StreamedCacheIndexTreeEntry<Key>& current_node = m_index_tree[current_idx];
                StreamedCacheIndexTreeEntry<Key>& grandparent_of_current_node = m_index_tree[grandparent_idx];
                StreamedCacheIndexTreeEntry<Key>& uncle_of_current_node = m_index_tree[uncle_idx];

                parent_of_current_node.node_color = 1;
                uncle_of_current_node.node_color = 1;
                grandparent_of_current_node.node_color = static_cast<unsigned char>(root_reached);

                current_idx = grandparent_idx;
                parent_idx = grandparent_of_current_node.parent_node;
                grandparent_idx = m_index_tree[parent_idx].parent_node;
                T1 = m_index_tree[parent_idx].inheritance_category
                    == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;
                uncle_idx = T1 ? m_index_tree[grandparent_idx].right_leaf : m_index_tree[grandparent_idx].left_leaf;
            }

            if (!root_reached)
            {
                // uncle is BLACK cases

                StreamedCacheIndexTreeEntry<Key>& parent_of_current_node = m_index_tree[parent_idx];
                StreamedCacheIndexTreeEntry<Key>& current_node = m_index_tree[current_idx];
                StreamedCacheIndexTreeEntry<Key>& grandparent_of_current_node = m_index_tree[grandparent_idx];

                bool T2 = current_node.inheritance_category
                    == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;

                if (T1 && T2)
                {
                    // Left-Left case
                    right_rotate(parent_idx, grandparent_idx);
                    swap_colors(parent_of_current_node.node_color,
                        grandparent_of_current_node.node_color);
                }
                else if (T1 && !T2)
                {
                    // Left-Right case
                    left_rotate(parent_idx, current_idx);
                    right_rotate(current_idx, grandparent_idx);
                    swap_colors(current_node.node_color,
                        grandparent_of_current_node.node_color);
                }
                else if (!T1 && T2)
                {
                    // Right-Left case
                    right_rotate(current_idx, parent_idx);
                    left_rotate(grandparent_idx, current_idx);
                    swap_colors(grandparent_of_current_node.node_color,
                        current_node.node_color);
                }
                else
                {
                    // Right-Right case
                    left_rotate(grandparent_idx, parent_idx);
                    swap_colors(grandparent_of_current_node.node_color,
                        parent_of_current_node.node_color);
                }
            }
        }
    }
}

template<typename Key>
inline bool StreamedCacheIndex<Key>::remove_entry(Key const& key)
{
    std::tuple<size_t, size_t, bool> d_and_s = bst_delete(key);
    if (!std::get<2>(d_and_s)) return false;


    size_t removed_node_idx = std::get<0>(d_and_s);
    size_t removed_node_sibling_idx = std::get<1>(d_and_s);

    StreamedCacheIndexTreeEntry<Key>& removed_node = m_index_tree[removed_node_idx];
    StreamedCacheIndexTreeEntry<Key>& removed_node_sibling = m_index_tree[removed_node_sibling_idx];

    size_t parent_of_removed_node_idx = removed_node_sibling.parent_node;
    StreamedCacheIndexTreeEntry<Key>& parent_of_removed_node = m_index_tree[parent_of_removed_node_idx];


    if (removed_node.node_color == 0 || parent_of_removed_node.node_color == 0)
    {
        // either the node to be removed or its parent is red (both cannot be red due to the RED-BLACK requirements)
        parent_of_removed_node.node_color = 1;
    }
    else
    {
        // both the node to be removed and its parent are black 
        removed_node.node_color = 2;    // "2" means the node is double-black

        size_t current_node_idx = removed_node_idx;
        size_t current_node_sibling_idx = removed_node_sibling_idx;
        size_t parent_of_current_node_idx = parent_of_removed_node_idx;
        while (current_node_idx && m_index_tree[current_node_idx].node_color == 2)
        {
            StreamedCacheIndexTreeEntry<Key>& current_node = m_index_tree[current_node_idx];
            StreamedCacheIndexTreeEntry<Key>& current_node_sibling = m_index_tree[current_node_sibling_idx];
            StreamedCacheIndexTreeEntry<Key>& parent_of_current_node = m_index_tree[parent_of_current_node_idx];

            // true when the sibling is the left child of its parent
            bool T1 = parent_of_current_node.left_leaf == current_node_sibling_idx;

            if (current_node_sibling.node_color == 1)
            {
                // sibling is BLACK

                // true if the left child of the sibling is RED
                bool T2 = current_node_sibling.left_leaf
                    && m_index_tree[current_node_sibling.left_leaf].node_color == 0;

                // true if the right child of the sibling is RED
                bool T3 = current_node_sibling.right_leaf
                    && m_index_tree[current_node_sibling.right_leaf].node_color == 0;


                if (T1 && T2)
                {
                    // left-left case
                    left_rotate(current_node_sibling_idx, parent_of_current_node_idx);
                    m_index_tree[current_node_sibling.left_leaf].node_color = 1;
                }
                else if (T1 && T3)
                {
                    // left-right case
                    size_t r_idx = current_node_sibling.right_leaf;
                    right_rotate(current_node_sibling_idx, r_idx);
                    left_rotate(r_idx, parent_of_current_node_idx);
                    m_index_tree[r_idx].node_color = 1;
                }
                else if (!T1 && T3)
                {
                    // right-right case
                    right_rotate(parent_of_current_node_idx, current_node_sibling_idx);
                    m_index_tree[current_node_sibling.right_leaf].node_color = 1;
                }
                else if (!T1 && T2)
                {
                    // right-left case
                    size_t r_idx = current_node_sibling.left_leaf;
                    left_rotate(r_idx, current_node_sibling_idx);
                    right_rotate(parent_of_current_node_idx, r_idx);
                    m_index_tree[r_idx].node_color = 1;
                }
                else
                {
                    // both of the sibling's children are BLACK
                    --current_node.node_color;
                    parent_of_current_node.node_color += current_node.node_color;

                    current_node_idx = parent_of_current_node_idx;
                    parent_of_current_node_idx = parent_of_current_node.parent_node;
                    current_node_sibling_idx =
                        parent_of_current_node.inheritance_category == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child
                        ? m_index_tree[parent_of_current_node_idx].right_leaf
                        : m_index_tree[parent_of_current_node_idx].left_leaf;
                }
            }
            else
            {
                // sibling is RED

                if (current_node_sibling.inheritance_category == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child)
                {
                    // left case
                    right_rotate(current_node_sibling_idx, parent_of_current_node_idx);
                    swap_colors(current_node_sibling.node_color, parent_of_current_node.node_color);
                    current_node_sibling_idx = parent_of_current_node.left_leaf;
                }
                else
                {
                    // right case
                    left_rotate(parent_of_current_node_idx, current_node_sibling_idx);
                    swap_colors(parent_of_current_node.node_color, current_node_sibling.node_color);
                    current_node_sibling_idx = parent_of_current_node.right_leaf;
                }
            }
        }

        if (!current_node_idx)
        {
            // we have reached the root of the tree, remove the double-black label
            --m_index_tree[0].node_color;
        }
    }

    ++m_current_index_redundant_growth_pressure;
    if (m_current_index_redundant_growth_pressure == m_max_index_redundant_growth_pressure) rebuild_index();
}

template<typename Key>
inline size_t core::StreamedCacheIndex<Key>::bst_insert(std::pair<Key, uint64_t> const& key_offset_pair)
{
    size_t const target_index = m_index_tree.size();

    StreamedCacheIndexTreeEntry<Key> new_entry;
    new_entry.data_offset = key_offset_pair.second;
    new_entry.cache_entry_key = key_offset_pair.first;
    new_entry.node_color = 0;    // newly inserted nodes are RED
    new_entry.left_leaf = 0;
    new_entry.right_leaf = 0;

    size_t insertion_node_idx;
    bool is_insertion_subtree_left;
    {
        size_t search_idx{ 0 };
        do
        {
            insertion_node_idx = search_idx;
            StreamedCacheIndexTreeEntry<Key>& current_node = m_index_tree[search_idx];
            search_idx = (is_insertion_subtree_left = new_entry.cache_entry_key < current_node.cache_entry_key)
                ? current_node.left_leaf
                : current_node.right_leaf;
        } while (search_idx);
    }

    // setup connection to the new node
    new_entry.parent_node = insertion_node_idx;
    if (is_insertion_subtree_left)
    {
        m_index_tree[insertion_node_idx].left_leaf = target_index;
        new_entry.inheritance_category = StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;
    }
    else
    {
        m_index_tree[insertion_node_idx].right_leaf = target_index;
        new_entry.inheritance_category = StreamedCacheIndexTreeEntry<Key>::inheritance_category::right_child;
    }

    // finally, physically insert new node into the tree buffer
    m_index_tree.push_back(new_entry);

    return insertion_node_idx;
}

template<typename Key>
inline void core::StreamedCacheIndex<Key>::swap_colors(unsigned char& color1, unsigned char& color2)
{
    color1 = color1^color2;
    color2 = color2^color1;
    color1 = color1^color2;
}

template<typename Key>
inline uint32_t StreamedCacheIndex<Key>::locate_bin(size_t n, std::vector<size_t> const& bins_in_accending_order)
{
    size_t left = 0U, right = bins_in_accending_order.size() - 1;

    while (right - left > 1)
    {
        size_t pivot = static_cast<size_t>(std::floor((right - left + 1) / 2.f));

        if (n < bins_in_accending_order[pivot])
            right = pivot;
        else
            left = pivot;
    }

    return left;
}

template<typename Key>
inline void StreamedCacheIndex<Key>::right_rotate(size_t a, size_t b)
{
    StreamedCacheIndexTreeEntry<Key>& node_a = m_index_tree[a];
    StreamedCacheIndexTreeEntry<Key>& node_b = m_index_tree[b];

    node_a.parent_node = node_b.parent_node;
    node_b.parent_node = a;
    node_b.left_leaf = node_a.right_leaf;
    node_a.right_leaf = b;
}

template<typename Key>
inline void StreamedCacheIndex<Key>::left_rotate(size_t a, size_t b)
{
    StreamedCacheIndexTreeEntry<Key>& node_a = m_index_tree[a];
    StreamedCacheIndexTreeEntry<Key>& node_b = m_index_tree[b];

    node_b.parent_node = node_a.parent_node;
    node_a.parent_node = b;
    node_a.right_leaf = node_b.left_leaf;
    node_b.left_leaf = a;
}

template<typename Key>
inline std::tuple<size_t, size_t, bool> StreamedCacheIndex<Key>::bst_delete(Key const& key)
{
    auto deletion_result = bst_search(key);
    if (!deletion_result.second)
        return std::make_tuple(static_cast<size_t>(0U), static_cast<size_t(0U), false);


    size_t node_to_delete_idx = deletion_result.first;
    size_t actually_removed_node_idx;
    size_t sibling_of_actually_removed_node_idx;

    StreamedCacheIndexTreeEntry<Key>& node_to_delete = m_index_tree[node_to_delete_idx];
    if (!node_to_delete.left_leaf && !node_to_delete.right_leaf)
    {
        // node to be deleted is a leaf

        StreamedCacheIndexTreeEntry<Key>& parent_node = m_index_tree[node_to_delete.parent_node];
        if (node_to_delete.inheritance_category == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child)
        {
            parent_node.left_leaf = 0;
            sibling_of_actually_removed_node_idx = parent_node.right_leaf;
        }
        else
        {
            parent_node.right_leaf = 0;
            sibling_of_actually_removed_node_idx = parent_node.left_leaf;
        }

        node_to_delete.to_be_deleted = true;
        actually_removed_node_idx = node_to_delete_idx;
    }
    else if (node_to_delete.left_leaf && !node_to_delete.right_leaf
        || !node_to_delete.left_leaf && node_to_delete.right_leaf)
    {
        // node to be deleted has only one child

        size_t child_node_idx = node_to_delete.left_leaf
            ? node_to_delete.left_leaf
            : node_to_delete.right_leaf;
        StreamedCacheIndexTreeEntry<Key>& child_node = m_index_tree[child_node_idx];

        if (node_to_delete)
        {
            // node to be deleted is not the root of the index tree

            size_t parent_node_idx = node_to_delete.parent_node;
            StreamedCacheIndexTreeEntry<Key>& parent_node = m_index_tree[parent_node_idx];

            if (parent_node.left_leaf == node_to_delete_idx)
            {
                parent_node.left_leaf = child_node_idx;
                sibling_of_actually_removed_node_idx = parent_node.right_leaf;
            }
            else
            {
                parent_node.right_leaf = child_node_idx;
                sibling_of_actually_removed_node_idx = parent_node.left_leaf;
            }
            child_node.parent_node = parent_node_idx;

            node_to_delete.to_be_deleted = true;
            actually_removed_node_idx = node_to_delete_idx;
        }
        else
        {
            // node to be deleted is the root of the index tree
            // here we will use the fact that we are indeed dealing with RED-BLACK trees, which
            // means that if the root of the tree has only one child (currently considered case),
            // then this child has to be RED and there are only two nodes left (including the root)

            node_to_delete.cache_entry_key = child_node.cache_entry_key;
            node_to_delete.data_offset = node_to_delete.data_offset;
            node_to_delete.left_leaf = node_to_delete.right_leaf = 0U;

            child_node.to_be_deleted = true;
            actually_removed_node_idx = child_node_idx;
            sibling_of_actually_removed_node_idx = 0U;    // does not matter in this case as the actually removed node is RED
        }
    }
    else
    {
        size_t in_order_successor_idx{ node_to_delete.right_leaf };
        {
            size_t aux;
            while (aux = m_index_tree[in_order_successor_idx].left_leaf)
                in_order_successor_idx = aux;
        }

        StreamedCacheIndexTreeEntry<Key>& in_order_successor_node = m_index_tree[in_order_successor_idx];
        node_to_delete.data_offset = in_order_successor_node.data_offset;
        node_to_delete.cache_entry_key = in_order_successor_node.cache_entry_key;

        size_t successor_right_child_idx = m_index_tree[in_order_successor_idx].right_leaf;
        size_t successor_parent_node_idx = m_index_tree[in_order_successor_idx].parent_node;
        StreamedCacheIndexTreeEntry<Key>& successor_parent_node = m_index_tree[successor_parent_node_idx];
        if (successor_right_child_idx)
        {
            StreamedCacheIndexTreeEntry<Key>& successor_child_node = m_index_tree[successor_right_child_idx];
            if (successor_parent_node.left_leaf == in_order_successor_idx)
            {
                successor_parent_node.left_leaf = successor_right_child_idx;
                sibling_of_actually_removed_node_idx = successor_parent_node.right_leaf;
            }
            else
            {
                successor_parent_node.right_leaf = successor_right_child_idx;
                sibling_of_actually_removed_node_idx = successor_parent_node.left_leaf;
            }
            successor_child_node.parent_node = successor_parent_node_idx;
        }
        else
        {
            if (successor_parent_node.left_leaf == in_order_successor_idx)
            {
                successor_parent_node.left_leaf = 0;
                sibling_of_actually_removed_node_idx = successor_parent_node.right_leaf;
            }
            else
            {
                successor_parent_node.right_leaf = 0;
                sibling_of_actually_removed_node_idx = successor_parent_node.left_leaf;
            }
        }

        in_order_successor_node.to_be_deleted = true;
        actually_removed_node_idx = in_order_successor_idx;
    }


    return std::make_tuple(actually_removed_node_idx, sibling_of_actually_removed_node_idx, true);
}

template<typename Key>
inline std::pair<size_t, bool> StreamedCacheIndex<Key>::bst_search(Key const& key)
{
    if (m_index_tree[0].to_be_deleted)
    {
        // the index tree is empty 
        return std::make_pair(static_cast<size_t>(0U), false);
    }


    size_t current_index = 0;
    do
    {
        StreamedCacheIndex<Key>& current_node = m_index_tree[current_index];

        if (key == current_node.cache_entry_key)
            return std::make_pair(current_index, true);

        current_index = key < current_node.cache_entry_key
            ? current_node.left_leaf
            : current_node.right_leaf;

    } while (current_index);

    return std::make_pair(static_cast<size_t>(0), false);
}

template<typename Key>
inline void StreamedCacheIndex<Key>::rebuild_index()
{
    std::vector<StreamedCacheIndexTreeEntry<Key>> new_index_tree_buffer;
    std::vector<size_t> to_be_deleted_indeces;
    new_index_tree_buffer.reserve(m_index_tree.size());
    to_be_deleted_indeces.reserve(m_index_tree.size() + 1U);

    to_be_deleted_indeces.push_back(0U);
    for (size_t i = 0U; i < m_index_tree.size(); ++i)
    {
        if (m_index_tree[i].to_be_deleted)
            to_be_deleted_indeces.push_back(i);
        else
            new_index_tree_buffer.push_back(m_index_tree[i]);
    }

    for (auto const& e : new_index_tree_buffer)
    {
        e.left_leaf -= locate_bin(e.left_leaf, to_be_deleted_indeces);
        e.right_leaf -= locate_bin(e.right_leaf, to_be_deleted_indeces);
        e.parent_node -= locate_bin(e.parent_node, to_be_deleted_indeces);
    }

    m_index_tree = std::move(new_index_tree_buffer);

    m_current_index_redundant_growth_pressure = 0U;
}



template<typename Key>
inline StreamedCacheIndexIterator& core::StreamedCacheIndex<Key>::StreamedCacheIndexIterator::operator++()
{
    if (m_has_reached_end)
        throw std::out_of_range{ "index tree iterator is out of range" };

    std::vector<StreamedCacheIndexTreeEntry<Key>>& index_tree = *m_p_target_index_tree;
    StreamedCacheIndexTreeEntry<Key>& current_node = index_tree[m_current_index];

    if (current_node.left_leaf)
    {
        m_current_index = current_node.left_leaf;
        return *this;
    }

    if (current_node.right_leaf)
    {
        m_current_index = current_node.right_leaf;
        return *this;
    }

    do
    {
        m_current_index = index_tree[m_current_index].parent_node;
    } while (m_current_index
        && (index_tree[m_current_index].inheritance
            == StreamedCacheIndexTreeEntry<Key>::inheritance_category::right_child
            || !index_tree[index_tree[m_current_index].parent_node].right_leaf));

    if (!m_current_index) m_has_reached_end = true;
    else m_current_index = index_tree[index_tree[m_current_index].parent_node].right_leaf;

    return *this;
}

template<typename Key>
inline StreamedCacheIndexIterator StreamedCacheIndex<Key>::StreamedCacheIndexIterator::operator++(int)
{
    StreamedCacheIndexIterator<Key> rv{ *this };
    ++(*this);
    return rv;
}

template<typename Key>
inline StreamedCacheIndexTreeEntry<Key>& StreamedCacheIndex<Key>::StreamedCacheIndexIterator::operator*()
{
    return (*m_p_target_index_tree)[m_current_index];
}

template<typename Key>
inline StreamedCacheIndexTreeEntry<Key> const& StreamedCacheIndex<Key>::StreamedCacheIndexIterator::operator*() const
{
    return const_cast<StreamedCacheIndex<Key>::StreamedCacheIndexIterator*>(this)->operator*();
}

template<typename Key>
inline StreamedCacheIndexTreeEntry<Key> const * StreamedCacheIndex<Key>::StreamedCacheIndexIterator::operator->() const
{
    return const_cast<StreamedCacheIndex<Key>::StreamedCacheIndexIterator*>(this)->operator->();
}

template<typename Key>
inline bool StreamedCacheIndex<Key>::StreamedCacheIndexIterator::operator==(StreamedCacheIndexIterator const& other) const
{
    return m_is_at_beginning == other.m_is_at_beginning
        && m_has_reached_end == other.m_has_reached_end
        && m_current_index == other.m_current_index
        && m_p_target_index_tree == other.m_p_target_index_tree;
}

template<typename Key>
inline bool StreamedCacheIndex<Key>::StreamedCacheIndexIterator::operator!=(StreamedCacheIndexIterator const& other) const
{
    return !(*this == other);
}

template<typename Key>
inline StreamedCacheIndexTreeEntry<Key>* StreamedCacheIndex<Key>::StreamedCacheIndexIterator::operator->()
{
    return &(*m_p_target_index_tree)[m_current_index];
}

template<typename Key>
inline StreamedCacheIndexIterator & StreamedCacheIndex<Key>::StreamedCacheIndexIterator::operator--()
{
    if (m_is_at_beginning)
        throw std::out_of_range{ "index tree iterator is out of range" };

    m_current_index = (*m_p_target_index_tree)[m_current_index].parent_node;
    if (!m_current_index) m_is_at_beginning = true;

    return *this;
}

template<typename Key>
inline StreamedCacheIndexIterator StreamedCacheIndex<Key>::StreamedCacheIndexIterator::operator--(int)
{
    StreamedCacheIndexIterator<Key> rv{ *this };
    --(*this);
    return rv;
}


template<typename Key>
inline void StreamedCacheIndexTreeEntry<Key>::prepare_serialization_blob(void* p_blob_memory)
{
    uint64_t* p_leaves_parent_and_data_offset = static_cast<uint64_t*>(p_blob_memory);
    p_leaves_parent_and_data_offset[0] = left_leaf;
    p_leaves_parent_and_data_offset[1] = right_leaf;
    p_leaves_parent_and_data_offset[2] = parent_node;
    p_leaves_parent_and_data_offset[3] = data_offset;

    void* p_key = static_cast<void*>(static_cast<uint64_t*>(p_blob_memory) + 4);
    cache_entry_key.serialize(p_key);

    unsigned char* p_color_inheritance_and_deletion_status =
        static_cast<unsigned char*>(p_key) + key_serialized_size;
    *p_color_inheritance_and_deletion_status = node_color | (inheritance << 2) | (static_cast<unsigned char>(to_be_deleted) << 4);
}

template<typename Key>
inline void StreamedCacheIndexTreeEntry<Key>::deserialize_from_blob(void* p_blob_memory)
{
    uint64_t* p_leaves_parent_and_data_offset = static_cast<uint64_t*>(p_blob_memory);
    left_leaf = p_leaves_parent_and_data_offset[0];
    right_leaf = p_leaves_parent_and_data_offset[1];
    parent_node = p_leaves_parent_and_data_offset[2];
    data_offset = p_leaves_parent_and_data_offset[3];

    void* p_key = static_cast<void*>(static_cast<uint64_t*>(p_blob_memory) + 4);
    cache_entry_key.deserialize(p_key);

    unsigned char* p_color_inheritance_and_deletion_status =
        static_cast<unsigned char*>(p_key) + key_serialized_size;
    node_color = (*p_color_inheritance_and_deletion_status & 0x3);
    inheritance = (*p_color_inheritance_and_deletion_status & 0xC) >> 2;
    to_be_deleted = (*p_color_inheritance_and_deletion_status & 0x10) != 0;
}

template<typename Key, size_t cluster_size>
inline void core::StreamedCache<Key, cluster_size>::serialize_entry(StreamedCacheEntry const& entry)
{
    size_t entry_size = m_data_blob_to_be_cached.size()
        + 8    // size of the entry field
        + 13;  // date stamp field

    size_t total_number_of_clusters = entry_size / cluster_size + static_cast<size_t>(entry_size % cluster_size > 0);
}

template<typename Key, size_t cluster_size>
inline core::StreamedCache<Key, cluster_size>::StreamedCache(std::iostream& cache_io_stream, size_t max_cache_size_in_bytes):
    m_cache_stream{ cache_io_stream },
    m_max_cache_size{ max_cache_size_in_bytes },
    m_current_cache_size{ 0U }
{
    if (!cache_io_stream)
    {
        misc::Log::retrieve()->out("Unable to open cache IO stream", misc::LogMessageType::error);
        return;
    }
}


}}
#endif
