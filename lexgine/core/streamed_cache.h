#ifndef LEXGINE_CORE_STREAMED_CACHE_H
#define LEXGINE_CORE_STREAMED_CACHE_H

#include <cstdint>
#include <memory>
#include <iostream>
#include <vector>
#include <mutex>
#include <iterator>
#include <numeric>

#include "../../3rd_party/zlib/zlib.h"

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
    friend class StreamedCache<Key, cluster_size>;

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

    static size_t const key_serialized_size = Key::serialized_size;
    static size_t const serialized_size = 32    //!< left and right leafs, parent node index, and the data offset
        + key_serialized_size    //!< size of serialized key
        + 1;    //!< node color (2 low order bits) + inheritance category (2 next bits) + is subject for deletion at some point (1 bit) + 3 currently unused bits   

    void prepare_serialization_blob(void* p_blob_memory);    //! serializes entry to memory provided (and owned) by the caller
    void deserialize_from_blob(void* p_blob_memory);    //! fills in the entry data based on serialized blob provided by the caller
};


//! Cache index structure
template<typename Key, size_t cluster_size>
class StreamedCacheIndex final
{
    friend class StreamedCache<Key, cluster_size>;


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

    size_t getCurrentRedundancy() const;    //! returns current redundancy of the index tree buffer represented in bytes

    size_t getMaxAllowedRedundancy() const;    //! returns maximal allowed redundancy of the tree buffer represented in bytes

    /*! Sets maximal redundancy allowed for the index tree buffer represented in bytes. Note that the real maximal allowed redundancy applied to the index tree
     buffer may be slightly less then the value provided to this function as the latter gets aligned to the size of certain internal structure representing
     single entry of the index tree. Call getMaxAllowedRedundancy() to get factual redundancy value applied to the index tree buffer
    */
    void setMaxAllowedRedundancy(size_t max_redundancy_in_bytes);

    size_t getSize() const;    //! returns total number of fields stored in the index tree

    size_t getNumberOfEntries() const;    //! returns number of fields in the index tree that are not "to be deleted"

    bool isEmpty() const;    //! returns 'true' if the index is empty; returns 'false' otherwise

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

private:
    misc::Optional<uint64_t> get_cache_entry_data_offset_from_key(Key const& key) const;    //! retrieves offset of cache entry in the associated stream based on provided key

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
    size_t m_current_index_redundant_growth_pressure = 0U;
    size_t m_max_index_redundant_growth_pressure = 1000U;    //!< maximal allowed amount of unused entries in the index tree buffer, after which the buffer is rebuilt
    size_t m_number_of_entries = 0U;    //!< number of living entities in the index tree (size of the tree excluding the entries that are marked as "to be deleted")
};


enum class StreamCacheCompressionLevel : int
{
    level0 = 0, level1, level2, level3, level4, level5, level6, level7, level8, level9
};

//! Class implementing main functionality for streamed data cache. This class is thread safe.
template<typename Key, size_t cluster_size = 16384U>
class StreamedCache : public NamedEntity<class_names::StreamedCache>
{
public:
    using key_type = Key;
    struct CustomHeader
    {
        static uint8_t const size = 32U;
        unsigned char data[size];
    };

public:
    //! initializes new cache
    StreamedCache(std::iostream& cache_io_stream, size_t capacity, 
       StreamCacheCompressionLevel compression_level = StreamCacheCompressionLevel::level0, bool are_overwrites_allowed = false);

    StreamedCache(std::iostream& cache_io_stream);    //! opens IO stream containing existing cache
    virtual ~StreamedCache();

    void addEntry(StreamedCacheEntry<Key, cluster_size> const& entry);    //! adds entry into the cache and immediately attempts to write it into the cache stream

    void finalize(); //! writes index data and empty cluster look-up table into the end of the cache stream and closes the stream before returning execution control to the caller

    size_t freeSpace() const;    //! returns space yet available to the cache

    size_t usedSpace() const;    //! returns space used by the cache so far

    size_t totalSpace() const;    //! total space allowed for the cache

    size_t hardSizeLimit() const;    //! returns total capacity of the cache plus maximal possible overhead. The cache cannot grow larger than this value.

    StreamedCacheEntry<Key, cluster_size> retrieveEntry(Key const& key) const;    //! retrieves an entry from the cache based on its key

    void removeEntry(Key const& key) const;    //! removes entry from the cache (and immediately from its associated stream) given its key

    StreamedCacheIndex const& getIndex() const;    //! returns index tree of the cache

    std::pair<uint16_t, uint16_t> getVersion() const;    //! returns major and minor versions of the cache (in this order) packed into std::pair

    void writeCustomHeader(CustomHeader const& custom_header);    //! writes custom header data into the cache
    CustomHeader retrieveCustomHeader() const;    //! retrieves custom header data from the cache

    bool isCompressed() const;    //! returns 'true' if the cache stream is compressed, returns 'false' otherwise

private:
    void write_header_data();

    std::pair<size_t, bool> serialize_entry(StreamedCacheEntry<Key, cluster_size> const& entry);
    StreamedCacheEntry<Key, cluster_size> deserialize_entry(size_t base_offset);

    static void pack_date_stamp(misc::DateTime const& date_stamp, char packed_date_stamp[13]) const;
    static misc::DateTime unpack_date_stamp(char packed_date_stamp[13]) const;
    static size_t align_to(size_t value, size_t alignment);

    std::pair<size_t, size_t> reserve_available_cluster_sequence(size_t size_hint);
    std::pair<size_t, size_t> allocate_space_in_cache(size_t size);
    std::pair<size_t, size_t> optimize_reservation(std::list<std::pair<size_t, size_t>>& reserved_sequence_list, size_t size_hint);
    void remove_oldest_entry_record();

private:
    uint32_t const m_version = 0x0100;    //!< hi-word contains major version number; lo-word contains the minor version
    uint8_t const m_cluster_overhead = 8U;
    uint8_t const m_sequence_overhead = 8U;
    uint8_t const m_entry_record_overhead = 13U;
    uint8_t const m_eclt_entry_size = 8U;

    uint32_t const m_header_size =
        4U    // cache version
        + 4U    // endiannes: 0x1234 for Big-Endian, 0x4321 for Little-Endian

        + 8U    // maximal allowed size of the cache represented in bytes
        + 8U    // current size of the cache body given in bytes (INCLUDING the cluster overhead)

        + 8U    // current total size of the index tree represented in bytes
        + 8U    // maximal allowed redundancy pressure in the index tree
        + 8U    // current redundancy pressure in the index tree

        + 8U    // current size of the empty cluster table represented in bytes

        + 1U    // flags (1st bit identifies whether the cache is compressed, 2nd defines whether overwrites are allowed, 6 bits are reserved)

        + CustomHeader::size;

    std::iostream& m_cache_stream;
    size_t m_max_cache_size;
    size_t m_cache_body_size;
    StreamedCacheIndex<Key, cluster_size> m_index;
    std::vector<size_t> m_empty_cluster_table;
    StreamCacheCompressionLevel m_compression_level;
    bool m_are_overwrites_allowed;   
    z_stream m_zlib_stream;
};



template<typename Key, size_t cluster_size>
inline StreamedCacheEntry<Key, cluster_size>::StreamedCacheEntry(Key const& key, DataBlob const& source_data_blob) :
    m_key{ key },
    m_data_blob_to_be_cached{ source_data_blob },
    m_date_stamp{ misc::DateTime::now() }
{

}

template<typename Key, size_t cluster_size>
inline size_t StreamedCacheIndex<Key, cluster_size>::getCurrentRedundancy() const
{
    return m_current_index_redundant_growth_pressure * StreamedCacheIndexTreeEntry<Key>::serialized_size;
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCacheIndex<Key, cluster_size>::getMaxAllowedRedundancy() const
{
    return m_max_index_redundant_growth_pressure * StreamedCacheIndexTreeEntry<Key>::serialized_size;
}

template<typename Key, size_t cluster_size>
inline void StreamedCacheIndex<Key, cluster_size>::setMaxAllowedRedundancy(size_t max_redundancy_in_bytes)
{
    m_max_index_redundant_growth_pressure = max_redundancy_in_bytes / StreamedCacheIndexTreeEntry<Key>::serialized_size;
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCacheIndex<Key, cluster_size>::getSize() const
{
    return m_index_tree.size()*StreamedCacheIndexTreeEntry<Key>::serialized_size;
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCacheIndex<Key, cluster_size>::getNumberOfEntries() const
{
    return m_number_of_entries;
}

template<typename Key, size_t cluster_size>
inline bool StreamedCacheIndex<Key, cluster_size>::isEmpty() const
{
    return m_index_tree.empty();
}

template<typename Key, size_t cluster_size>
inline iterator StreamedCacheIndex<Key, cluster_size>::begin()
{
    iterator rv{};
    rv.m_is_at_beginning = true;
    rv.m_has_reached_end = false;
    rv.m_p_target_index_tree = &m_index_tree;
    return rv;
}

template<typename Key, size_t cluster_size>
inline iterator StreamedCacheIndex<Key, cluster_size>::end()
{
    iterator rv{};
    rv.m_p_target_index_tree = &m_index_tree;
    return rv;
}

template<typename Key, size_t cluster_size>
inline const_iterator StreamedCacheIndex<Key, cluster_size>::begin() const
{
    return const_cast<StreamedCacheIndex<Key, cluster_size>*>(this)->begin();
}

template<typename Key, size_t cluster_size>
inline const_iterator StreamedCacheIndex<Key, cluster_size>::end() const
{
    return const_cast<StreamedCacheIndex<Key, cluster_size>*>(this)->end();
}

template<typename Key, size_t cluster_size>
inline const_iterator StreamedCacheIndex<Key, cluster_size>::cbegin() const
{
    return begin();
}

template<typename Key, size_t cluster_size>
inline const_iterator StreamedCacheIndex<Key, cluster_size>::cend() const
{
    return end();
}

template<typename Key, size_t cluster_size>
inline misc::Optional<uint64_t> StreamedCacheIndex<Key, cluster_size>::get_cache_entry_data_offset_from_key(Key const& key) const
{
    std::pair<size_t, bool> search_result = bst_search(key);
    if (search_result.second)
        return misc::Optional<uint64_t>{ m_index_tree[search_result.first].data_offset };

    return misc::Optional<uint64_t>{};
}

template<typename Key, size_t cluster_size>
inline void StreamedCacheIndex<Key, cluster_size>::add_entry(std::pair<Key, uint64_t> const& key_offset_pair)
{
    if (!m_index_tree.size())
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

    ++m_number_of_entries;
}

template<typename Key, size_t cluster_size>
inline bool StreamedCacheIndex<Key, cluster_size>::remove_entry(Key const& key)
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
    if (m_current_index_redundant_growth_pressure == m_max_index_redundant_growth_pressure
        || !removed_node_idx)
    {
        rebuild_index();
    }

    --m_number_of_entries;
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCacheIndex<Key, cluster_size>::bst_insert(std::pair<Key, uint64_t> const& key_offset_pair)
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

template<typename Key, size_t cluster_size>
inline void StreamedCacheIndex<Key, cluster_size>::swap_colors(unsigned char& color1, unsigned char& color2)
{
    color1 = color1^color2;
    color2 = color2^color1;
    color1 = color1^color2;
}

template<typename Key, size_t cluster_size>
inline uint32_t StreamedCacheIndex<Key, cluster_size>::locate_bin(size_t n, std::vector<size_t> const& bins_in_accending_order)
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

template<typename Key, size_t cluster_size>
inline void StreamedCacheIndex<Key, cluster_size>::right_rotate(size_t a, size_t b)
{
    StreamedCacheIndexTreeEntry<Key>& node_a = m_index_tree[a];
    StreamedCacheIndexTreeEntry<Key>& node_b = m_index_tree[b];

    node_a.parent_node = node_b.parent_node;
    node_b.parent_node = a;
    node_b.left_leaf = node_a.right_leaf;
    node_a.right_leaf = b;
}

template<typename Key, size_t cluster_size>
inline void StreamedCacheIndex<Key, cluster_size>::left_rotate(size_t a, size_t b)
{
    StreamedCacheIndexTreeEntry<Key>& node_a = m_index_tree[a];
    StreamedCacheIndexTreeEntry<Key>& node_b = m_index_tree[b];

    node_b.parent_node = node_a.parent_node;
    node_a.parent_node = b;
    node_a.right_leaf = node_b.left_leaf;
    node_b.left_leaf = a;
}

template<typename Key, size_t cluster_size>
inline std::tuple<size_t, size_t, bool> StreamedCacheIndex<Key, cluster_size>::bst_delete(Key const& key)
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

template<typename Key, size_t cluster_size>
inline std::pair<size_t, bool> StreamedCacheIndex<Key, cluster_size>::bst_search(Key const& key)
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

template<typename Key, size_t cluster_size>
inline void StreamedCacheIndex<Key, cluster_size>::rebuild_index()
{
    if (m_index_tree[0].to_be_deleted)
    {
        m_current_index_redundant_growth_pressure = 0U;
        m_index_tree.clear();
        return;
    }


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



template<typename Key, size_t cluster_size>
inline StreamedCacheIndexIterator& StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator++()
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

template<typename Key, size_t cluster_size>
inline StreamedCacheIndexIterator StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator++(int)
{
    StreamedCacheIndexIterator<Key> rv{ *this };
    ++(*this);
    return rv;
}

template<typename Key, size_t cluster_size>
inline StreamedCacheIndexTreeEntry<Key>& StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator*()
{
    return (*m_p_target_index_tree)[m_current_index];
}

template<typename Key, size_t cluster_size>
inline StreamedCacheIndexTreeEntry<Key> const& StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator*() const
{
    return const_cast<StreamedCacheIndex<Key>::StreamedCacheIndexIterator*>(this)->operator*();
}

template<typename Key, size_t cluster_size>
inline StreamedCacheIndexTreeEntry<Key> const* StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator->() const
{
    return const_cast<StreamedCacheIndex<Key>::StreamedCacheIndexIterator*>(this)->operator->();
}

template<typename Key, size_t cluster_size>
inline bool StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator==(StreamedCacheIndexIterator const& other) const
{
    return m_is_at_beginning == other.m_is_at_beginning
        && m_has_reached_end == other.m_has_reached_end
        && m_current_index == other.m_current_index
        && m_p_target_index_tree == other.m_p_target_index_tree;
}

template<typename Key, size_t cluster_size>
inline bool StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator!=(StreamedCacheIndexIterator const& other) const
{
    return !(*this == other);
}

template<typename Key, size_t cluster_size>
inline StreamedCacheIndexTreeEntry<Key>* StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator->()
{
    return &(*m_p_target_index_tree)[m_current_index];
}

template<typename Key, size_t cluster_size>
inline StreamedCacheIndexIterator& StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator--()
{
    if (m_is_at_beginning)
        throw std::out_of_range{ "index tree iterator is out of range" };

    m_current_index = (*m_p_target_index_tree)[m_current_index].parent_node;
    if (!m_current_index) m_is_at_beginning = true;

    return *this;
}

template<typename Key, size_t cluster_size>
inline StreamedCacheIndexIterator StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator--(int)
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
inline bool StreamedCache<Key, cluster_size>::isCompressed() const
{
    return static_cast<int>(m_compression_level) > 0;
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::pack_date_stamp(misc::DateTime const& date_stamp, char packed_date_stamp[13]) const
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
inline misc::DateTime StreamedCache<Key, cluster_size>::unpack_date_stamp(char packed_date_stamp[13]) const
{
    uint16_t year = *static_cast<uint16_t*>(packed_date_stamp);
    uint8_t month = packed_date_stamp[2] & 0xF;
    uint8_t day = packed_date_stamp[2] >> 4; day |= (packed_date_stamp[3] & 0x1) << 4;
    uint8_t hour = (packed_date_stamp[3] & 0x3E) >> 1;
    uint8_t minute = packed_date_stamp[3] >> 6; minute |= packed_date_stamp[4] << 2;
    double second = *static_cast<double*>(packed_date_stamp + 5);

    return misc::DateTime{ year, month, day, hour, minute, second };
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCache<Key, cluster_size>::align_to(size_t value, size_t alignment)
{
    std::div_t aux = std::div(value, alignment);
    return (aux.quot + static_cast<size_t>(aux.rem != 0))*alignment;
}

template<typename Key, size_t cluster_size>
inline std::pair<size_t, bool> core::StreamedCache<Key, cluster_size>::serialize_entry(StreamedCacheEntry<Key, cluster_size> const& entry)
{
    std::unique_ptr<DataBlob> blob_to_serialize_ptr{ nullptr };
    if (isCompressed())
    {
        m_zlib_stream.next_in = static_cast<Bytef*>(entry.m_data_blob_to_be_cached.data());
        m_zlib_stream.avail_in = static_cast<uInt>(entry.m_data_blob_to_be_cached.size());

        uLong deflated_entry_size = deflateBound(m_zlib_stream, static_cast<uLong>(entry.m_data_blob_to_be_cached.size()));

        blob_to_serialize_ptr.reset(new DataChunk{ static_cast<size_t>(deflated_entry_size) });
        m_zlib_stream.next_out = static_cast<Bytef*>(blob_to_serialize_ptr->data());
        m_zlib_stream.avail_out = deflated_entry_size;

        if (deflate(m_zlib_stream, Z_FINISH) != Z_STREAM_END)
        {
            misc::Log::retrieve()->out("Unable to compress entry record during serialization to streamed cache \""
                + getStringName() + "\" (zlib deflate() error", misc::LogMessageType::error);
            return std::make_pair(0U, false);
        }
    }
    else
    {
        blob_to_serialize_ptr.reset(new DataBlob{ entry.m_data_blob_to_be_cached.data(), entry.m_data_blob_to_be_cached.size() });
    }

    size_t entry_size = m_entry_record_overhead + blob_to_serialize_ptr->size();
    std::pair<size_t, size_t> cache_allocation_desc = allocate_space_in_cache(entry_size);



    std::streampos old_writing_position = m_cache_stream.tellp();
    m_cache_stream.seekp(cache_allocation_desc.first + m_sequence_overhead, std::ios::beg);

    char packed_date_stamp[13U];
    pack_date_stamp(entry.m_date_stamp, packed_date_stamp);
    m_cache_stream.write(packed_date_stamp, m_entry_record_overhead);

    uint64_t current_cluster_base_address{ cache_allocation_desc.first + m_sequence_overhead + m_entry_record_overhead };
    size_t num_bytes_to_write_into_current_cluster{ cluster_size - m_sequence_overhead - m_entry_record_overhead };
    size_t total_bytes_left_to_write = blob_to_serialize_ptr->size();
    size_t total_bytes_written{ 0U };
    std::streampos old_reading_position = m_cache_stream.tellg();
    while (total_bytes_left_to_write)
    {
        m_cache_stream.seekp(current_cluster_base_address, std::ios::beg);
        m_cache_stream.write(static_cast<char*>(blob_to_serialize_ptr->data()) + total_bytes_written, 
            num_bytes_to_write_into_current_cluster);

        m_cache_stream.seekg(m_cache_stream.tellp());
        m_cache_stream.read(reinterpret_cast<char*>(&current_cluster_base_address));

        total_bytes_left_to_write -= num_bytes_to_write_into_current_cluster;
        total_bytes_written += num_bytes_to_write_into_current_cluster;
        num_bytes_to_write_into_current_cluster = std::min(total_bytes_left_to_write, cluster_size);
    }

    m_cache_stream.seekg(old_reading_position);
    m_cache_stream.seekp(old_writing_position);

    return std::make_pair(cache_allocation_desc.first, true);
}

template<typename Key, size_t cluster_size>
inline StreamedCacheEntry<Key, cluster_size> StreamedCache<Key, cluster_size>::deserialize_entry(size_t base_offset)
{
    
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::write_header_data()
{
    std::streampos old_stream_writing_position = m_cache_stream.tellp();

    m_cache_stream.seekp(0, std::ios::beg);
    m_cache_stream.write(reinterpret_cast<char*>(&m_version), 4U);

    union {
        uint32_t flag;
        char bytes[4];
    }endiannes;
    endiannes.flag = 0x1234;
    m_cache_stream.write(endiannes.bytes, 4U);

    uint64_t aux;

    aux = m_max_cache_size; m_cache_stream.write(reinterpret_cast<char*>(&aux), 8U);
    aux = m_cache_body_size; m_cache_stream.write(reinterpret_cast<char*>(&aux), 8U);

    uint64_t size_of_index_tree = m_index.getSize();
    m_cache_stream.write(reinterpret_cast<char*>(&size_of_index_tree), 8U);

    aux = m_index.m_max_index_redundant_growth_pressure; m_cache_stream.write(reinterpret_cast<char*>(&aux), 8U);
    aux = m_index.m_current_index_redundant_growth_pressure; m_cache_stream.write(reinterpret_cast<char*>(&aux), 8U);

    uint64_t size_of_empty_cluster_table = m_empty_cluster_table.size() * m_eclt_entry_size;
    m_cache_stream.write(reinterpret_cast<char*>(&size_of_empty_cluster_table), 8U);

    char flags = static_cast<uint64_t>(isCompressed()) | static_cast<uint64_t>(m_are_overwrites_allowed) << 1;
    m_cache_stream.write(&flags, 1U);

    m_cache_stream.seekp(old_stream_writing_position);
}

template<typename Key, size_t cluster_size>
inline std::pair<size_t, size_t> StreamedCache<Key, cluster_size>::reserve_available_cluster_sequence(size_t size_hint)
{
    if(m_empty_cluster_table.size())
    {
        std::streampos old_stream_reading_position = m_cache_stream.tellg();

        size_t base_offset = m_empty_cluster_table.back(); m_empty_cluster_table.pop_back();
        m_cache_stream.seekg(base_offset, std::ios::beg);

        uint64_t cluster_sequence_length;
        m_cache_stream.read(reinterpret_cast<char*>(&cluster_sequence_length), 8U);

        m_cache_stream.seekg(old_stream_reading_position);
        
        return std::make_pair(base_offset, static_cast<size_t>(cluster_sequence_length));
    }
    

    size_t num_unpartitioned_clusters = (m_max_cache_size - m_cache_body_size) / (cluster_size + m_cluster_overhead);
    if (!num_unpartitioned_clusters) return std::make_pair<size_t, size_t>(0U, 0U);

    size_t new_sequence_base_offset = m_header_size + m_cache_body_size; 
    uint64_t new_sequence_length = std::min(
        num_unpartitioned_clusters,
        align_to(size_hint, cluster_size) / cluster_size);

    std::streampos old_stream_writing_position = m_cache_stream.tellp();
    m_cache_stream.seekp(new_sequence_base_offset, std::ios::beg);
    m_cache_stream.write(reinterpret_cast<char*>(&new_sequence_length), 8U);

    uint64_t cluster_base_offset{ new_sequence_base_offset };
    for(size_t i = 0; i < new_sequence_length - 1; ++i, cluster_base_offset += cluster_size + m_cluster_overhead)
    {
        uint64_t next_cluster_base_offset = cluster_base_offset + cluster_size + m_cluster_overhead;
        m_cache_stream.seekp(cluster_base_offset + cluster_size, std::ios::beg);
        m_cache_stream.write(reinterpret_cast<char*>(&next_cluster_base_offset), 8U);
    }
    m_cache_body_size += new_sequence_length*(cluster_size + m_cluster_overhead);
    m_cache_stream.seekp(old_stream_writing_position);

    return std::make_pair(new_sequence_base_offset, static_cast<size_t>(new_sequence_length));
}

template<typename Key, size_t cluster_size>
inline std::pair<size_t, size_t> StreamedCache<Key, cluster_size>::allocate_space_in_cache(size_t size)
{
    std::list<std::pair<size_t, size_t>> reserved_sequence_list{};
    size_t allocated_so_far{ 0U };
    size_t requested_capacity_plus_overhead = size + m_sequence_overhead;

    size_t max_allocation_size = align_to(requested_capacity_plus_overhead + m_cluster_overhead, cluster_size + m_cluster_overhead);
    if (max_allocation_size > m_max_cache_size) return std::make_pair<size_t, size_t>(0U, 0U);

    while (allocated_so_far < requested_capacity_plus_overhead)
    {
        std::pair<size_t, size_t> cluster_sequence_desc = reserve_available_cluster_sequence(requested_capacity_plus_overhead - allocated_so_far);
        if (!cluster_sequence_desc.second)
        {
            // the cache is exhausted
            if (m_are_overwrites_allowed) remove_oldest_entry_record();
            else break;
        }

        allocated_so_far += cluster_sequence_desc.second*cluster_size;
        reserved_sequence_list.push_back(cluster_sequence_desc);
    }

    return reserved_sequence_list.size()
        ? optimize_reservation(reserved_sequence_list, requested_capacity_plus_overhead)
        : std::make_pair<size_t, size_t>(0U, 0U);
}

template<typename Key, size_t cluster_size>
inline std::pair<size_t, size_t> StreamedCache<Key, cluster_size>::optimize_reservation(
    std::list<std::pair<size_t, size_t>>& reserved_sequence_list, 
    size_t size_hint)
{
    std::streampos old_writing_position = m_cache_stream.tellp();
    uint64_t total_sequence_length{ 0U };
    for(auto p = reserved_sequence_list.begin(); p != reserved_sequence_list.end(); ++p)
    {
        std::list<std::pair<size_t, size_t>>::iterator q{ p }; ++q;
        uint64_t next_sequence_base_address = q != reserved_sequence_list.end() ? q->first : 0U;

        m_cache_stream.seekp(p->first + p->second*(cluster_size + m_cluster_overhead) - m_cluster_overhead, std::ios::beg);
        m_cache_stream.write(reinterpret_cast<char*>(&next_sequence_base_address), 8U);

        total_sequence_length += p->second;
    }

    // remove redundant clusters if possible
    size_t total_sequence_capacity = total_sequence_length * cluster_size;
    if (total_sequence_capacity > size_hint)
    {
        size_t redundant_space = total_sequence_capacity - size_hint;
        size_t redundant_sequence_length = redundant_space / cluster_size;

        if(redundant_sequence_length)
        {
            auto& last_sequence_desc = reserved_sequence_list.back();
            size_t contracted_sequence_length = last_sequence_desc.second - redundant_sequence_length;
            size_t dissected_sequence_base_address = last_sequence_desc.first
                + contracted_sequence_length*(cluster_size + m_cluster_overhead);

            m_cache_stream.seekp(dissected_sequence_base_address - m_cluster_overhead, std::ios::beg);
            uint64_t aux{ 0U }; m_cache_stream.write(reinterpret_cast<char*>(&aux), 8U);

            m_cache_stream.seekp(dissected_sequence_base_address, std::ios::beg);
            aux = redundant_sequence_length; m_cache_stream.write(reinterpret_cast<char*>(&aux), 8U);
            m_empty_cluster_table.push_back(dissected_sequence_base_address);

            total_sequence_length -= redundant_sequence_length;
        }
    }

    m_cache_stream.seekp(reserved_sequence_list.front().first, std::ios::beg);
    m_cache_stream.write(reinterpret_cast<char*>(&total_sequence_length), 8U);
    m_cache_stream.seekp(old_writing_position);

    return std::make_pair(reserved_sequence_list.front().first, static_cast<size_t>(total_sequence_length));
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::remove_oldest_entry_record()
{
    std::streampos old_reading_position = m_cache_stream.tellg();
    misc::DateTime oldest_entry_datestampt{ 0xFF, 12, 31, 23, 59, 59.9999 };
    StreamedCacheIndexTreeEntry<Key>* p_oldest_entry{ nullptr };
    for (StreamedCacheIndexTreeEntry<Key>& e : m_index)
    {
        m_cache_stream.seekg(e.data_offset, std::ios::beg);
        char packed_date_stamp[13];
        m_cache_stream.read(packed_date_stamp, 13);
        DateTime unpacked_date_stamp = unpack_date_stamp(packed_date_stamp);
        if (unpacked_date_stamp < oldest_entry_datestampt)
        {
            oldest_entry_datestampt = unpacked_date_stamp;
            p_oldest_entry = &e;
        }
    }
    
    m_index.remove_entry(p_oldest_entry->cache_entry_key);
    m_empty_cluster_table.push_back(static_cast<size_t>(p_oldest_entry->data_offset));

    m_cache_stream.seekg(old_reading_position);
}

template<typename Key, size_t cluster_size>
inline StreamedCache<Key, cluster_size>::StreamedCache(std::iostream& cache_io_stream, size_t capacity, 
    StreamCacheCompressionLevel compression_level/* = StreamCacheCompressionLevel::level0*/, bool are_overwrites_allowed/* = false*/):
    m_cache_stream{ cache_io_stream },
    m_max_cache_size{ align_to(align_to(capacity, cluster_size) / cluster_size
    * (cluster_size + m_cluster_overhead + m_sequence_overhead + m_entry_record_overhead), cluster_size + m_cluster_overhead) },
    m_cache_body_size{ 0U },
    m_compression_level{ compression_level },
    m_are_overwrites_allowed{ are_overwrites_allowed }
{
    if (!cache_io_stream)
    {
        misc::Log::retrieve()->out("Unable to open cache IO stream", misc::LogMessageType::error);
        return;
    }

    // reserve space for the cache header
    cache_io_stream.seekp(m_header_size, std::ios::beg);

    if (isCompressed())
    {
        m_zlib_stream.zalloc = Z_NULL;
        m_zlib_stream.zfree = Z_NULL;

        int rv = deflateInit(&m_zlib_stream, static_cast<int>(m_compression_level));
        if (rv != Z_OK)
        {
            m_compression_level = StreamCacheCompressionLevel::level0;
            switch (rv)
            {
            case Z_MEM_ERROR:
                misc::Log::retrieve()->out("Not enough memory to initialize zlib. The streamed cache \"" + getStringName() +
                    "\" will default to uncompressed state", misc::LogMessageType::exclamation);
                break;

            case Z_STREAM_ERROR:
                misc::Log::retrieve()->out("zlib was provided with incorrect compression level of \"" +
                    std::to_string(static_cast<int>(m_compression_level)) + "\". The streamed cache \"" + getStringName() +
                    "\" will default to uncompressed state", misc::LogMessageType::exclamation);
                break;

            case Z_VERSION_ERROR:
                misc::Log::retrieve()->out("zlib binary version differs from the version assumed by the caller."
                    " The streamed cache \"" + getStringName() +
                    "\" will default to uncompressed state", misc::LogMessageType::exclamation);
                break;
            }
        }
    }
}



template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::addEntry(StreamedCacheEntry<Key, cluster_size> const &entry)
{
    std::pair<size_t, bool> rv = serialize_entry(entry);
    if (!rv.second)
    {
        misc::Log::retrieve()->out("Unable to serialize entry to cache \"" + getStringName() + "\"", misc::LogMessageType::error);
        return;
    }

    m_index.add_entry(std::make_pair(entry.m_key, rv.first));
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCache<Key, cluster_size>::freeSpace() const
{
    return m_max_cache_size - usedSpace();
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCache<Key, cluster_size>::usedSpace() const
{
    std::streampos old_read_position = m_cache_stream.tellg();
    size_t emptied_cluster_sequences_total_capacity{ 0U };
    for (size_t addr : m_empty_cluster_table)
    {
        m_cache_stream.seekg(addr, std::ios::beg);
        uint64_t cluster_sequence_length;
        m_cache_stream.read(reinterpret_cast<char*>(&cluster_sequence_length), 8U);
        emptied_cluster_sequences_total_capacity += static_cast<size_t>(cluster_sequence_length)*cluster_size;
    }
    m_cache_stream.seekg(old_read_position);

    size_t unpartitioned_cache_size = m_max_cache_size - m_cache_body_size;
    size_t cluster_reservation_overhead = unpartitioned_cache_size / (cluster_size + m_cluster_overhead) * m_cluster_overhead;
    return std::min(m_max_cache_size,
        m_cache_body_size - emptied_cluster_sequences_total_capacity
        + cluster_reservation_overhead + m_sequence_overhead + m_entry_record_overhead);
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCache<Key, cluster_size>::totalSpace() const
{
    size_t cluster_reservation_overhead = m_max_cache_size / (cluster_size + m_cluster_overhead) * m_cluster_overhead;
    return m_max_cache_size - cluster_reservation_overhead - m_sequence_overhead - m_entry_record_overhead;
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCache<Key, cluster_size>::hardSizeLimit() const
{
    return m_max_cache_size + m_max_cache_size / (cluster_size + m_cluster_overhead) * StreamedCacheIndexTreeEntry<Key>::serialized_size;
}

template<typename Key, size_t cluster_size>
inline StreamedCacheIndex const & StreamedCache<Key, cluster_size>::getIndex() const
{
    return m_index;
}


}}
#endif
