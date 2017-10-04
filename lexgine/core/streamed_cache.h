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


//! Single 16KB cluster of the cache
struct CacheCluster final
{
    char data[16384];
    uint64_t next_cluster_offset;
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
    public:
        // required by output iterator standard behavior

        StreamedCacheIndexIterator(StreamedCacheIndexIterator const& other);
        StreamedCacheIndexIterator& operator++();
        StreamedCacheIndexIterator operator++(int);
        StreamedCacheIndexTreeEntry<Key>& operator*();
        

        // required by input iterator standard behavior

        StreamedCacheIndexTreeEntry<Key> const& operator*() const;
        StreamedCacheIndexTreeEntry<Key> const* operator->() const;
        bool operator==(StreamedCacheIndexIterator const& other) const;
        bool operator!=(StreamedCacheIndexIterator const& other) const;


        // required by forward iterator standard behavior
        StreamedCacheIndexIterator();
        StreamedCacheIndexTreeEntry<Key>* operator->();
        StreamedCacheIndexIterator& operator=(StreamedCacheIndexIterator const& other);


        // required by bidirectional iterator standard behavior
        StreamedCacheIndexIterator& operator--();
        StreamedCacheIndexIterator operator--(int);

    private:
        size_t m_current_index;
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

private:
    void add_entry(std::pair<Key, uint64_t> const& key_offset_pair);    //! adds entry into cache index tree
    bool remove_entry(Key const& key);    //! removes entry from cache index tree


    size_t bst_insert(std::pair<Key, uint64_t> const& key_offset_pair);    //! standard BST-insertion without RED-BLACK properties check
    static void swap_colors(unsigned char& color1, unsigned char& color2);

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
template<typename Key>
class StreamedCache : public NamedEntity<class_names::StreamedCache>
{
public:
    using key_type = Key;

public:
    StreamedCache(std::ostream& cache_output_stream, size_t max_cache_size_in_bytes);
    virtual ~StreamedCache();

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
inline void core::StreamedCacheIndex<Key>::add_entry(std::pair<Key, uint64_t> const& key_offset_pair)
{
    if (m_index_tree.size() == 0)
    {
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

        if (m_index_tree[parent_idx].node_color == 0)
        {
            // parent is RED:
            // in this case the RED-BLACK structure of the tree is corrupted and needs to be recovered

            size_t grandparent_idx = m_index_tree[parent_idx].parent_node;
            bool T1 = m_index_tree[parent_idx].inheritance 
                == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;    
            size_t uncle_idx = T1 ? m_index_tree[grandparent_idx].right_leaf : m_index_tree[grandparent_idx].left_leaf;

            bool root_reached{ false };
            while (!root_reached && uncle_idx && m_index_tree[uncle_idx].node_color == 0)
            {
                // uncle is RED case
                root_reached = grandparent_idx == 0;
                m_index_tree[parent_idx].node_color = 1;
                m_index_tree[uncle_idx].node_color = 1;
                m_index_tree[grandparent_idx].node_color = static_cast<unsigned char>(root_reached);

                current_idx = grandparent_idx;
                parent_idx = m_index_tree[current_idx].parent_node;
                grandparent_idx = m_index_tree[parent_idx].parent_node;
                T1 = m_index_tree[parent_idx].inheritance_category 
                    == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;
                uncle_idx = T1 ? m_index_tree[grandparent_idx].right_leaf : m_index_tree[grandparent_idx].left_leaf;
            }

            if (!root_reached)
            {
                // uncle is BLACK cases

                bool T2 = m_index_tree[current_idx].inheritance_category 
                    == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;

                if (T1 && T2)
                {
                    // Left-Left case
                    right_rotate(parent_idx, grandparent_idx);
                    swap_colors(m_index_tree[parent_idx].node_color,
                        m_index_tree[grandparent_idx].node_color);
                }
                else if (T1 && !T2)
                {
                    // Left-Right case
                    left_rotate(parent_idx, current_idx);
                    right_rotate(current_idx, grandparent_idx);
                    swap_colors(m_index_tree[current_idx].node_color,
                        m_index_tree[grandparent_idx].node_color);
                }
                else if (!T1 && T2)
                {
                    // Right-Left case
                    right_rotate(current_idx, parent_idx);
                    left_rotate(grandparent_idx, current_idx);
                    swap_colors(m_index_tree[grandparent_idx].node_color,
                        m_index_tree[current_idx].node_color);
                }
                else
                {
                    // Right-Right case
                    left_rotate(grandparent_idx, parent_idx);
                    swap_colors(m_index_tree[grandparent_idx].node_color,
                        m_index_tree[parent_idx].node_color);
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
    }else
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
    }else if (node_to_delete.left_leaf && !node_to_delete.right_leaf
        || !node_to_delete.left_leaf && node_to_delete.right_leaf)
    {
        // node to be deleted has only one child

        size_t child_node_idx = node_to_delete.left_leaf 
            ? node_to_delete.left_leaf 
            : node_to_delete.right_leaf;

        size_t parent_node_idx = node_to_delete.parent_node;
        StreamedCacheIndexTreeEntry<Key>& parent_node = m_index_tree[parent_node_idx];
        StreamedCacheIndexTreeEntry<Key>& child_node = m_index_tree[child_node_idx];
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
    // !!!TO BE IMPLEMENTED!!!

    std::vector<StreamedCacheIndexTreeEntry> new_index_tree_buffer;
    new_index_tree_buffer.reserve(m_index_tree.size());

    for (auto& entry : m_index_tree)
    {
        if (!entry.to_be_deleted)
            new_index_tree_buffer.push_back(entry);
    }

}


}}

#endif
