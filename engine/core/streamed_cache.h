#ifndef LEXGINE_CORE_STREAMED_CACHE_H
#define LEXGINE_CORE_STREAMED_CACHE_H

#include <cstdint>
#include <memory>
#include <fstream>
#include <vector>
#include <mutex>
#include <iterator>
#include <numeric>
#include <cassert>

#include "3rd_party/zlib/zlib.h"

#include "data_blob.h"
#include "misc/datetime.h"
#include "entity.h"
#include "class_names.h"
#include "misc/optional.h"

namespace lexgine::core
{

template<typename Key, size_t cluster_size> class StreamedCache;

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

    class StreamedCacheIndexIterator
    {
        friend class StreamedCacheIndex;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = StreamedCacheIndexTreeEntry<Key>;
        using difference_type = ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

    public:
        // required by output iterator standard behavior

        StreamedCacheIndexIterator(StreamedCacheIndexIterator const&) = default;
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
        StreamedCacheIndexIterator& operator=(StreamedCacheIndexIterator const&) = default;


        // required by bidirectional iterator standard behavior

        StreamedCacheIndexIterator& operator--();
        StreamedCacheIndexIterator operator--(int);

    private:
        size_t subtree_get_leftmost_node(size_t subtree_root) const;
        size_t subtree_get_rightmost_node(size_t subtree_root) const;

    private:
        bool m_is_at_beginning = true;
        bool m_has_reached_end = true;
        size_t m_current_index = static_cast<size_t>(-1);
        std::vector<StreamedCacheIndexTreeEntry<Key>>* m_p_target_index_tree = nullptr;
    };

    class StreamedCacheIndexConstIterator
    {
        friend class StreamedCacheIndex;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = StreamedCacheIndexTreeEntry<Key>;
        using difference_type = ptrdiff_t;
        using pointer = value_type const*;
        using reference = value_type const&;

    public:
        // required by input iterator standard behavior

        StreamedCacheIndexConstIterator(StreamedCacheIndexConstIterator const&) = default;

        StreamedCacheIndexConstIterator& operator++();
        StreamedCacheIndexConstIterator operator++(int);

        StreamedCacheIndexTreeEntry<Key> const& operator*() const;
        StreamedCacheIndexTreeEntry<Key> const* operator->() const;

        bool operator==(StreamedCacheIndexConstIterator const& other) const;
        bool operator!=(StreamedCacheIndexConstIterator const& other) const;


        // required by forward iterator standard behavior

        StreamedCacheIndexConstIterator() = default;
        StreamedCacheIndexConstIterator& operator=(StreamedCacheIndexConstIterator const&) = default;


        // required by bidirectional iterator standard behavior

        StreamedCacheIndexConstIterator& operator--();
        StreamedCacheIndexConstIterator operator--(int);

    private:
        StreamedCacheIndexIterator m_iter;
    };

public:

    using key_type = Key;
    using iterator = StreamedCacheIndexIterator;
    using const_iterator = StreamedCacheIndexConstIterator;

public:

    size_t getCurrentRedundancy() const;    //! returns current redundancy of the index tree buffer represented in bytes

    size_t getMaxAllowedRedundancy() const;    //! returns maximal allowed redundancy of the tree buffer represented in bytes

    /*! Sets maximal redundancy allowed for the index tree buffer represented in bytes. Note that the real maximal allowed redundancy applied to the index tree
     buffer may be slightly less then the value provided to this function as the latter gets aligned to the size of certain internal structure representing
     single entry of the index tree. Call getMaxAllowedRedundancy() to get factual redundancy value applied to the index tree buffer
    */
    void setMaxAllowedRedundancy(size_t max_redundancy_in_bytes) const;

    size_t getSize() const;    //! returns total number of fields stored in the index tree

    size_t getNumberOfEntries() const;    //! returns number of fields in the index tree that are not "to be deleted"

    bool isEmpty() const;    //! returns 'true' if the index is empty; returns 'false' otherwise

    /*! Generates representation of the underlying RED-BLACK tree structure using DOT graph description language and
     stores this representation into provided destination file. This function is useful for debugging purposes and
     for analyzing cached content as well as associated look-up overhead
    */
    void generateDOTRepresentation(std::string const& destination_file) const;

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
    template<typename T1, typename T2>
    static void swap_values(T1& value1, T2& value2);
    static size_t locate_bin(size_t n, std::vector<size_t> const& bins_in_accending_order);

    void right_rotate(size_t& a, size_t& b);
    void left_rotate(size_t& a, size_t& b);

    /*! standard BST deletion based on provided key. This function returns tuple containing index of the node that gets replaced and the
     index of its sibling node. The third and the last item of the tuple will be set to 'true' if deletion was successful; otherwise it will be set to 'false'
    */
    std::tuple<size_t, size_t, bool> bst_delete(Key const& key);

    std::pair<size_t, bool> bst_search(Key const& key) const;    //! retrieves address of the node having the given key. The second element of returned pair is 'true' if the node has been found and 'false' otherwise.

    void rebuild_index();    //! builds new index tree buffer cleaned up from unused entries

private:

    static size_t const m_key_size = Key::serialized_size;
    std::vector<StreamedCacheIndexTreeEntry<Key>> m_index_tree;
    size_t m_current_index_redundant_growth_pressure = 0U;
    size_t mutable m_max_index_redundant_growth_pressure = 1000U;    //!< maximal allowed amount of unused entries in the index tree buffer, after which the buffer is rebuilt
    size_t m_number_of_entries = 0U;    //!< number of living entities in the index tree (size of the tree excluding the entries that are marked as "to be deleted")
};


enum class StreamedCacheCompressionLevel : int
{
    level0 = 0, level1, level2, level3, level4, level5, level6, level7, level8, level9
};

//! Class implementing main functionality for streamed data cache. This class is thread safe.
template<typename Key, size_t cluster_size = 4096U>
class StreamedCache : public NamedEntity<class_names::StreamedCache>
{
public:
    using key_type = Key;
    struct CustomHeader
    {
        static uint8_t constexpr size = 32U;
        unsigned char data[size];
    };

    using entry_type = StreamedCacheEntry<Key, cluster_size>;
    using key_type = Key;

public:
    //! initializes new cache
    StreamedCache(std::iostream& cache_io_stream, size_t capacity,
        StreamedCacheCompressionLevel compression_level = StreamedCacheCompressionLevel::level0, bool are_overwrites_allowed = false);

    StreamedCache(std::iostream& cache_io_stream, bool read_only = false);    //! loads existing cache from provided IO stream

    StreamedCache(StreamedCache const&) = delete;
    StreamedCache(StreamedCache&& other);

    virtual ~StreamedCache();

    bool addEntry(entry_type const& entry, bool force_overwrite = false);    //! adds entry into the cache and immediately attempts to write it into the cache stream

    void finalize(); //! writes index data and empty cluster look-up table into the end of the cache stream and closes the stream before returning execution control to the caller

    size_t freeSpace() const;    //! returns space yet available to the cache

    size_t usedSpace() const;    //! returns space used by the cache so far

    size_t totalSpace() const;    //! total space allowed for the cache

    size_t hardSizeLimit() const;    //! returns total capacity of the cache plus maximal possible overhead. The cache cannot grow larger than this value.

    SharedDataChunk retrieveEntry(Key const& entry_key, size_t* valid_bytes_count = nullptr) const;    //! retrieves an entry from the cache based on its key

    misc::DateTime getEntryTimestamp(Key const& entry_key) const;    //! retrieves time stamp of entry with given key

    size_t getEntrySize(Key const& entry_key) const;    //! retrieves uncompressed size of entry with given key

    bool removeEntry(Key const& entry_key);    //! removes entry from the cache (and immediately from its associated stream) given its key

    bool doesEntryExist(Key const& entry_key) const;    //! returns 'true' if entry with requested key exists in the cache; returns 'false' otherwise

    StreamedCacheIndex<Key, cluster_size> const& getIndex() const;    //! returns index tree of the cache

    std::pair<uint16_t, uint16_t> getVersion() const;    //! returns major and minor versions of the cache (in this order) packed into std::pair

    bool writeCustomHeader(CustomHeader const& custom_header);    //! writes custom header data into the cache
    CustomHeader retrieveCustomHeader() const;    //! retrieves custom header data from the cache

    bool isCompressed() const;    //! returns 'true' if the cache stream is compressed, returns 'false' otherwise

    bool isGood() const;    //! returns 'true' if the cache has been successfully initialized

    operator bool() const;    //! same as isGood()

private:
    void write_header_data();
    void write_index_data();
    void write_eclt_data();

    void load_service_data();
    void load_index_data(size_t index_tree_size_in_bytes);
    void load_eclt_data(size_t eclt_data_size_in_bytes);

private:
    std::pair<size_t, bool> serialize_entry(StreamedCacheEntry<Key, cluster_size> const& entry, size_t overwrite_address);
    std::pair<SharedDataChunk, size_t> deserialize_entry(size_t data_offset) const;

private:
    static void pack_date_stamp(misc::DateTime const& date_stamp, unsigned char packed_date_stamp[13]);
    static misc::DateTime unpack_date_stamp(unsigned char packed_date_stamp[13]);
    static size_t align_to(size_t value, size_t alignment);

private:
    std::pair<size_t, size_t> reserve_available_cluster_sequence(size_t size_hint);
    std::pair<size_t, size_t> allocate_space_in_cache(size_t size);

    // Retrieves base offset of the cluster having given index withing provided cluster sequence
    size_t get_cluster_base_address(std::pair<size_t, size_t> const& sequence_allocation_desc, size_t cluster_idx);

    std::pair<size_t, size_t> optimize_reservation(std::list<std::pair<size_t, size_t>>& reserved_sequence_list, size_t size_hint);
    void remove_oldest_entry_record();

private:
    static char constexpr m_magic_bytes[] = { 'L', 'X', 'G', 'C' };
    static uint32_t constexpr m_version = 0x10000;    //!< hi-word contains major version number; lo-word contains the minor version
    static uint8_t constexpr m_cluster_overhead = 8U;
    static uint8_t constexpr m_sequence_overhead = 8U;
    static uint8_t constexpr m_datestamp_size = 13U;
    static uint8_t constexpr m_uncompressed_size_record = 8U;
    static uint8_t constexpr m_entry_record_overhead = m_uncompressed_size_record + m_datestamp_size;    // initialized in constructor that creates new cache
    static uint8_t constexpr m_eclt_entry_size = 8U;

    static uint32_t constexpr m_header_size =
        4U      // magic bytes    
        + 4U    // cache version
        + 4U    // endiannes: 0x1234 for Big-Endian, 0x4321 for Little-Endian

        + 8U    // maximal allowed size of the cache represented in bytes
        + 8U    // current size of the cache body given in bytes (INCLUDING the cluster overhead)

        + 8U    // current total size of the index tree represented in bytes
        + 8U    // maximal allowed redundancy pressure in the index tree
        + 8U    // current redundancy pressure in the index tree

        + 8U    // current size of the empty cluster table represented in bytes

        + 1U    // flags (first 4 bits identify compression level of the cache, the 5th bit defines whether overwrites are allowed, 3 bits are reserved)

        + CustomHeader::size;

    std::iostream& m_cache_stream;
    size_t m_max_cache_size;
    size_t m_cache_body_size;
    StreamedCacheIndex<Key, cluster_size> m_index;
    std::vector<size_t> m_empty_cluster_table;
    StreamedCacheCompressionLevel m_compression_level;
    bool m_are_overwrites_allowed;
    bool m_endianness_conversion_required;
    bool m_is_finalized;
    std::unique_ptr<DataChunk> m_aux_compression_buffer;
    bool m_is_read_only;
    bool m_is_good;
};



struct Int64Key final
{
    uint64_t value;

    static size_t const serialized_size = 8U;

    std::string toString() const { return std::to_string(value); }

    void serialize(void* p_serialization_blob) const
    {
        *(reinterpret_cast<uint64_t*>(p_serialization_blob)) = value;
    }

    void deserialize(void const* p_serialization_blob)
    {
        value = *(reinterpret_cast<uint64_t const*>(p_serialization_blob));
    }

    Int64Key(uint64_t value) : value{ value } {}

    Int64Key() = default;

    bool operator<(Int64Key const& other) const { return value < other.value; }
    bool operator==(Int64Key const& other) const { return value == other.value; }
};


using StreamedCache_KeyInt64_Cluster16KB = StreamedCache<Int64Key, 16384>;
using StreamedCache_KeyInt64_Cluster8KB = StreamedCache<Int64Key, 8192>;
using StreamedCache_KeyInt64_Cluster4KB = StreamedCache<Int64Key, 4096>;


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
inline void StreamedCacheIndex<Key, cluster_size>::setMaxAllowedRedundancy(size_t max_redundancy_in_bytes) const
{
    m_max_index_redundant_growth_pressure = max_redundancy_in_bytes / StreamedCacheIndexTreeEntry<Key>::serialized_size;

}

template<typename Key, size_t cluster_size>
inline size_t StreamedCacheIndex<Key, cluster_size>::getSize() const
{
    return m_index_tree.size() * StreamedCacheIndexTreeEntry<Key>::serialized_size;
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
inline void StreamedCacheIndex<Key, cluster_size>::generateDOTRepresentation(std::string const& destination_file) const
{
    std::ofstream ofile{ destination_file };
    if (!ofile)
    {
        misc::Log::retrieve()->out("Unable to open destination file \""
            + destination_file + "\" to store DOT representation of the index tree", misc::LogMessageType::error);
        return;
    }

    ofile << "digraph {" << std::endl;
    ofile << "node[style=filled];" << std::endl;
    for (StreamedCacheIndexTreeEntry<Key> const& n : *this)
    {
        std::string current_node_name{ "node" + std::to_string(n.data_offset) };
        ofile << current_node_name << "[label=" << n.cache_entry_key.toString() << ", "
            << "shape=circle, fontcolor=white, fillcolor=" << (n.node_color ? "black];" : "red];")
            << std::endl;
    }
    for (StreamedCacheIndexTreeEntry<Key> const& n : *this)
    {
        if (n.left_leaf)
        {
            ofile << "node" << std::to_string(n.data_offset) << "->"
                << "node" << std::to_string(m_index_tree[n.left_leaf].data_offset) << ";" << std::endl;
        }

        if (n.right_leaf)
        {
            ofile << "node" << std::to_string(n.data_offset) << "->"
                << "node" << std::to_string(m_index_tree[n.right_leaf].data_offset) << ";" << std::endl;
        }
    }

    ofile << "}" << std::endl;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::iterator StreamedCacheIndex<Key, cluster_size>::begin()
{
    iterator rv{};
    rv.m_has_reached_end = false;
    rv.m_p_target_index_tree = &m_index_tree;
    rv.m_current_index = rv.subtree_get_leftmost_node(0);

    return rv;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::iterator StreamedCacheIndex<Key, cluster_size>::end()
{
    iterator rv{};
    rv.m_is_at_beginning = false;
    rv.m_p_target_index_tree = &m_index_tree;
    rv.m_current_index = rv.subtree_get_rightmost_node(0);

    return rv;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::const_iterator StreamedCacheIndex<Key, cluster_size>::begin() const
{
    const_iterator rv{};
    rv.m_iter = const_cast<StreamedCacheIndex<Key, cluster_size>&>(*this).begin();

    return rv;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::const_iterator StreamedCacheIndex<Key, cluster_size>::end() const
{
    const_iterator rv{};
    rv.m_iter = const_cast<StreamedCacheIndex<Key, cluster_size>&>(*this).end();

    return rv;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::const_iterator StreamedCacheIndex<Key, cluster_size>::cbegin() const
{
    return begin();
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::const_iterator StreamedCacheIndex<Key, cluster_size>::cend() const
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
    if (!m_number_of_entries)
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
        StreamedCacheIndexTreeEntry<Key>* p_parent_of_current_node = &m_index_tree[parent_idx];
        StreamedCacheIndexTreeEntry<Key>& inserted_node = m_index_tree[current_idx];

        size_t grandparent_idx = p_parent_of_current_node->parent_node;
        StreamedCacheIndexTreeEntry<Key>& grandparent_of_inserted_node = m_index_tree[grandparent_idx];

        bool T1 = p_parent_of_current_node->inheritance
            == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;
        size_t uncle_idx = T1 ? grandparent_of_inserted_node.right_leaf : grandparent_of_inserted_node.left_leaf;

        bool is_parent_black{ false };
        while (!(is_parent_black = p_parent_of_current_node->node_color == 1))
        {
            StreamedCacheIndexTreeEntry<Key>& current_node = m_index_tree[current_idx];
            StreamedCacheIndexTreeEntry<Key>& grandparent_of_current_node = m_index_tree[grandparent_idx];
            StreamedCacheIndexTreeEntry<Key>& uncle_of_current_node = m_index_tree[uncle_idx];

            if (uncle_of_current_node.node_color == 0)
            {
                // parent is RED and uncle is RED
                p_parent_of_current_node->node_color = 1;
                uncle_of_current_node.node_color = 1;
                grandparent_of_current_node.node_color = static_cast<unsigned char>(grandparent_idx == 0);

                current_idx = grandparent_idx;
                parent_idx = grandparent_of_current_node.parent_node;
                p_parent_of_current_node = &m_index_tree[parent_idx];
                grandparent_idx = p_parent_of_current_node->parent_node;

                T1 = m_index_tree[parent_idx].inheritance
                    == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;
                uncle_idx = T1 ? m_index_tree[grandparent_idx].right_leaf : m_index_tree[grandparent_idx].left_leaf;
            }
            else
            {
                // parent is RED and uncle is BLACK case

                bool T2 = current_node.inheritance
                    == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;

                if (T1 && T2)
                {
                    // Left-Left case
                    swap_values(p_parent_of_current_node->node_color,
                        grandparent_of_current_node.node_color);
                    right_rotate(parent_idx, grandparent_idx);
                }
                else if (T1 && !T2)
                {
                    // Left-Right case
                    swap_values(current_node.node_color,
                        grandparent_of_current_node.node_color);
                    left_rotate(parent_idx, current_idx);
                    right_rotate(current_idx, grandparent_idx);
                }
                else if (!T1 && T2)
                {
                    // Right-Left case
                    swap_values(grandparent_of_current_node.node_color,
                        current_node.node_color);
                    right_rotate(current_idx, parent_idx);
                    left_rotate(grandparent_idx, current_idx);
                }
                else
                {
                    // Right-Right case
                    swap_values(grandparent_of_current_node.node_color,
                        p_parent_of_current_node->node_color);
                    left_rotate(grandparent_idx, parent_idx);
                }

                break;    // we can break the loop at this point
            }
        }

    }

    ++m_number_of_entries;
}

template<typename Key, size_t cluster_size>
inline bool StreamedCacheIndex<Key, cluster_size>::remove_entry(Key const& key)
{
    std::tuple<size_t, size_t, bool> d_s_and_success_flag = bst_delete(key);
    if (!std::get<2>(d_s_and_success_flag)) return false;

    size_t replaced_node_idx = std::get<0>(d_s_and_success_flag);
    size_t replaced_node_sibling_idx = std::get<1>(d_s_and_success_flag);

    StreamedCacheIndexTreeEntry<Key>& replaced_node = m_index_tree[replaced_node_idx];
    size_t parent_of_replaced_node_idx = replaced_node.parent_node;

    if (replaced_node.right_leaf || replaced_node.left_leaf || replaced_node.node_color == 0)
    {
        // we get here iff the node actually being removed is a leaf (iff it's RED) or a single-child parent (must be BLACK with RED child, which is necessarily a leaf node)
        if (replaced_node.right_leaf) m_index_tree[replaced_node.right_leaf].node_color = 1;
        if (replaced_node.left_leaf) m_index_tree[replaced_node.left_leaf].node_color = 1;
    }
    else
    {
        // the node being actually removed from the tree is a black leaf

        // replacing node is always NIL in this case, therefore we temporarily mark the zombie node that is getting replaced as double-black
        replaced_node.node_color = 2;

        size_t current_node_idx = replaced_node_idx;
        size_t current_node_sibling_idx = replaced_node_sibling_idx;
        size_t parent_of_current_node_idx = parent_of_replaced_node_idx;
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

                if (T1 && T3)
                {
                    // left-right case

                    size_t r_idx = current_node_sibling.right_leaf;

                    m_index_tree[r_idx].node_color = parent_of_current_node.node_color;
                    parent_of_current_node.node_color = 1;
                    current_node.node_color = 1;

                    left_rotate(current_node_sibling_idx, r_idx);
                    right_rotate(r_idx, parent_of_current_node_idx);
                }
                else if (T1 && T2)
                {
                    // left-left case

                    m_index_tree[current_node_sibling.left_leaf].node_color = parent_of_current_node.node_color;
                    current_node.node_color = 1;

                    right_rotate(current_node_sibling_idx, parent_of_current_node_idx);
                }
                else if (!T1 && T2)
                {
                    // right-left case

                    size_t r_idx = current_node_sibling.left_leaf;

                    m_index_tree[r_idx].node_color = parent_of_current_node.node_color;
                    parent_of_current_node.node_color = 1;
                    current_node.node_color = 1;

                    right_rotate(r_idx, current_node_sibling_idx);
                    left_rotate(parent_of_current_node_idx, r_idx);
                }
                else if (!T1 && T3)
                {
                    // right-right case

                    m_index_tree[current_node_sibling.right_leaf].node_color = parent_of_current_node.node_color;
                    current_node.node_color = 1;

                    left_rotate(parent_of_current_node_idx, current_node_sibling_idx);
                }
                else
                {
                    // both of the sibling's children are BLACK
                    --current_node.node_color;
                    parent_of_current_node.node_color += current_node.node_color;
                    current_node_sibling.node_color = 0;

                    current_node_idx = parent_of_current_node_idx;
                    parent_of_current_node_idx = parent_of_current_node.parent_node;
                    current_node_sibling_idx =
                        parent_of_current_node.inheritance == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child
                        ? m_index_tree[parent_of_current_node_idx].right_leaf
                        : m_index_tree[parent_of_current_node_idx].left_leaf;
                }
            }
            else
            {
                // sibling is RED

                if (current_node_sibling.inheritance == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child)
                {
                    // left case
                    swap_values(current_node_sibling.node_color, parent_of_current_node.node_color);
                    right_rotate(current_node_sibling_idx, parent_of_current_node_idx);
                    current_node_sibling_idx = m_index_tree[parent_of_current_node_idx].left_leaf;
                }
                else
                {
                    // right case
                    swap_values(parent_of_current_node.node_color, current_node_sibling.node_color);
                    left_rotate(parent_of_current_node_idx, current_node_sibling_idx);
                    current_node_sibling_idx = m_index_tree[parent_of_current_node_idx].right_leaf;
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
        || !replaced_node_idx)
    {
        rebuild_index();
    }

    --m_number_of_entries;

    assert(m_index_tree[0].node_color);    // root cannot become RED

    return true;
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
        new_entry.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;
    }
    else
    {
        m_index_tree[insertion_node_idx].right_leaf = target_index;
        new_entry.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::right_child;
    }

    // finally, physically insert new node into the tree buffer
    m_index_tree.push_back(new_entry);

    return insertion_node_idx;
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCacheIndex<Key, cluster_size>::locate_bin(size_t n, std::vector<size_t> const& bins_in_accending_order)
{
    size_t left = 0U, right = bins_in_accending_order.size() - 1;

    while (right - left != 1)
    {
        size_t pivot = static_cast<size_t>(std::floor((right + left) / 2.f));

        if (n < bins_in_accending_order[pivot])
            right = pivot;
        else
            left = pivot;
    }

    return left;
}

template<typename Key, size_t cluster_size>
inline void StreamedCacheIndex<Key, cluster_size>::right_rotate(size_t& a, size_t& b)
{
    StreamedCacheIndexTreeEntry<Key>& node_a = m_index_tree[a];
    StreamedCacheIndexTreeEntry<Key>& node_b = m_index_tree[b];

    if (node_b.inheritance == StreamedCacheIndexTreeEntry<Key>::inheritance_category::root)
    {
        // swap nodes a and b (everything, but inheritance and adjacency)
        swap_values(node_a.to_be_deleted, node_b.to_be_deleted);
        swap_values(node_a.data_offset, node_b.data_offset);
        std::swap(node_a.cache_entry_key, node_b.cache_entry_key);
        swap_values(node_a.node_color, node_b.node_color);

        swap_values(node_b.left_leaf, node_b.right_leaf);
        node_a.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::right_child;

        swap_values(node_b.left_leaf, node_a.left_leaf);
        m_index_tree[node_b.left_leaf].parent_node = 0;
        if (node_a.left_leaf) m_index_tree[node_a.left_leaf].parent_node = a;

        swap_values(node_a.right_leaf, node_a.left_leaf);
        if (node_a.left_leaf)
        {
            StreamedCacheIndexTreeEntry<Key>& n = m_index_tree[node_a.left_leaf];
            n.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;
            n.parent_node = a;
        }

        swap_values(a, b);
    }
    else
    {
        node_a.inheritance = node_b.inheritance;
        node_b.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::right_child;

        node_a.parent_node = node_b.parent_node;
        node_b.parent_node = a;

        if (node_a.inheritance == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child)
            m_index_tree[node_a.parent_node].left_leaf = a;
        else
            m_index_tree[node_a.parent_node].right_leaf = a;

        node_b.left_leaf = node_a.right_leaf;
        node_a.right_leaf = b;

        if (node_b.left_leaf)
        {
            StreamedCacheIndexTreeEntry<Key>& n = m_index_tree[node_b.left_leaf];
            n.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;
            n.parent_node = b;
        }
    }
}

template<typename Key, size_t cluster_size>
inline void StreamedCacheIndex<Key, cluster_size>::left_rotate(size_t& a, size_t& b)
{
    StreamedCacheIndexTreeEntry<Key>& node_a = m_index_tree[a];
    StreamedCacheIndexTreeEntry<Key>& node_b = m_index_tree[b];

    if (node_a.inheritance == StreamedCacheIndexTreeEntry<Key>::inheritance_category::root)
    {
        // swap nodes a and b (everything, but inheritance and adjacency)
        swap_values(node_a.to_be_deleted, node_b.to_be_deleted);
        swap_values(node_a.data_offset, node_b.data_offset);
        std::swap(node_a.cache_entry_key, node_b.cache_entry_key);
        swap_values(node_a.node_color, node_b.node_color);

        swap_values(node_a.left_leaf, node_a.right_leaf);
        node_b.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;

        swap_values(node_a.right_leaf, node_b.right_leaf);
        m_index_tree[node_a.right_leaf].parent_node = 0;
        if (node_b.right_leaf) m_index_tree[node_b.right_leaf].parent_node = b;

        swap_values(node_b.left_leaf, node_b.right_leaf);
        if (node_b.right_leaf)
        {
            StreamedCacheIndexTreeEntry<Key>& n = m_index_tree[node_b.right_leaf];
            n.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::right_child;
            n.parent_node = b;
        }

        swap_values(a, b);
    }
    else
    {
        node_b.inheritance = node_a.inheritance;
        node_a.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;

        node_b.parent_node = node_a.parent_node;
        node_a.parent_node = b;

        if (node_b.inheritance == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child)
            m_index_tree[node_b.parent_node].left_leaf = b;
        else
            m_index_tree[node_b.parent_node].right_leaf = b;

        node_a.right_leaf = node_b.left_leaf;
        node_b.left_leaf = a;

        if (node_a.right_leaf)
        {
            StreamedCacheIndexTreeEntry<Key>& n = m_index_tree[node_a.right_leaf];
            n.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::right_child;
            n.parent_node = a;
        }
    }
}

template<typename Key, size_t cluster_size>
inline std::tuple<size_t, size_t, bool> StreamedCacheIndex<Key, cluster_size>::bst_delete(Key const& key)
{
    auto deletion_result = bst_search(key);
    if (!deletion_result.second)
        return std::make_tuple(static_cast<size_t>(0U), static_cast<size_t>(0U), false);

    size_t node_to_delete_idx = deletion_result.first;
    size_t replaced_node_idx;
    size_t replaced_node_sibling_idx;

    StreamedCacheIndexTreeEntry<Key>& node_to_delete = m_index_tree[node_to_delete_idx];
    if (!node_to_delete.left_leaf && !node_to_delete.right_leaf)
    {
        // node to be deleted is a leaf

        StreamedCacheIndexTreeEntry<Key>& parent_node = m_index_tree[node_to_delete.parent_node];
        if (node_to_delete.inheritance == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child)
        {
            parent_node.left_leaf = 0;
            replaced_node_sibling_idx = parent_node.right_leaf;
        }
        else
        {
            parent_node.right_leaf = 0;
            replaced_node_sibling_idx = parent_node.left_leaf;
        }

        node_to_delete.to_be_deleted = true;
        replaced_node_idx = node_to_delete_idx;
    }
    else if (node_to_delete.left_leaf && !node_to_delete.right_leaf
        || !node_to_delete.left_leaf && node_to_delete.right_leaf)
    {
        // node to be deleted has only one child

        size_t child_node_idx = node_to_delete.left_leaf
            ? node_to_delete.left_leaf
            : node_to_delete.right_leaf;
        StreamedCacheIndexTreeEntry<Key>& child_node = m_index_tree[child_node_idx];

        if (node_to_delete_idx)
        {
            // node to be deleted is not the root of the index tree

            size_t parent_node_idx = node_to_delete.parent_node;
            StreamedCacheIndexTreeEntry<Key>& parent_node = m_index_tree[parent_node_idx];

            if (parent_node.left_leaf == node_to_delete_idx)
            {
                parent_node.left_leaf = child_node_idx;
                replaced_node_sibling_idx = parent_node.right_leaf;
                child_node.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;
            }
            else
            {
                parent_node.right_leaf = child_node_idx;
                replaced_node_sibling_idx = parent_node.left_leaf;
                child_node.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::right_child;
            }
            child_node.parent_node = parent_node_idx;

            node_to_delete.to_be_deleted = true;
            replaced_node_idx = node_to_delete_idx;
        }
        else
        {
            // node to be deleted is the root of the index tree
            // here we will use the fact that we are indeed dealing with RED-BLACK trees, which
            // means that if the root of the tree has only one child (currently considered case),
            // then this child has to be RED and there are only two nodes left (including the root)

            node_to_delete.cache_entry_key = child_node.cache_entry_key;
            node_to_delete.data_offset = child_node.data_offset;
            node_to_delete.left_leaf = node_to_delete.right_leaf = 0U;

            child_node.to_be_deleted = true;
            replaced_node_idx = node_to_delete_idx;
            replaced_node_sibling_idx = 0U;    // does not matter in this case as the actually removed node is RED
        }
    }
    else
    {
        size_t in_order_successor_node_idx{ node_to_delete.right_leaf };
        {
            size_t aux;
            while (aux = m_index_tree[in_order_successor_node_idx].left_leaf)
                in_order_successor_node_idx = aux;
        }

        StreamedCacheIndexTreeEntry<Key>& in_order_successor_node = m_index_tree[in_order_successor_node_idx];
        node_to_delete.data_offset = in_order_successor_node.data_offset;
        node_to_delete.cache_entry_key = in_order_successor_node.cache_entry_key;

        size_t successor_right_child_idx = m_index_tree[in_order_successor_node_idx].right_leaf;
        size_t successor_parent_node_idx = m_index_tree[in_order_successor_node_idx].parent_node;
        StreamedCacheIndexTreeEntry<Key>& successor_parent_node = m_index_tree[successor_parent_node_idx];

        // note: if in order successor of the node to be deleted has offspring nodes, they must be on its right side
        if (successor_right_child_idx)
        {
            StreamedCacheIndexTreeEntry<Key>& successor_child_node = m_index_tree[successor_right_child_idx];

            if (successor_parent_node.left_leaf == in_order_successor_node_idx)
            {
                successor_parent_node.left_leaf = successor_right_child_idx;
                replaced_node_sibling_idx = successor_parent_node.right_leaf;
                successor_child_node.inheritance = StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child;
            }
            else
            {
                successor_parent_node.right_leaf = successor_right_child_idx;
                replaced_node_sibling_idx = successor_parent_node.left_leaf;
            }
            successor_child_node.parent_node = successor_parent_node_idx;
        }
        else
        {
            // if in order successor does not have a right leaf, it doesn't have leaves at all

            if (successor_parent_node.left_leaf == in_order_successor_node_idx)
            {
                successor_parent_node.left_leaf = 0;
                replaced_node_sibling_idx = successor_parent_node.right_leaf;
            }
            else
            {
                successor_parent_node.right_leaf = 0;
                replaced_node_sibling_idx = successor_parent_node.left_leaf;
            }
        }

        in_order_successor_node.to_be_deleted = true;
        replaced_node_idx = in_order_successor_node_idx;
    }


    return std::make_tuple(replaced_node_idx, replaced_node_sibling_idx, true);
}

template<typename Key, size_t cluster_size>
inline std::pair<size_t, bool> StreamedCacheIndex<Key, cluster_size>::bst_search(Key const& key) const
{
    if (!m_index_tree.size() || m_index_tree[0].to_be_deleted)
    {
        // the index tree is empty 
        return std::make_pair(static_cast<size_t>(0U), false);
    }


    size_t current_index = 0;
    do
    {
        StreamedCacheIndexTreeEntry<Key> const& current_node = m_index_tree[current_index];

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
    to_be_deleted_indeces.push_back(m_index_tree.size());

    for (auto& e : new_index_tree_buffer)
    {
        e.left_leaf -= locate_bin(e.left_leaf, to_be_deleted_indeces);
        e.right_leaf -= locate_bin(e.right_leaf, to_be_deleted_indeces);
        e.parent_node -= locate_bin(e.parent_node, to_be_deleted_indeces);
    }

    m_index_tree = std::move(new_index_tree_buffer);

    m_current_index_redundant_growth_pressure = 0U;
}



template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator& StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator++()
{
    if (m_has_reached_end)
        throw std::out_of_range{ "index tree iterator is out of range" };

    std::vector<StreamedCacheIndexTreeEntry<Key>>& index_tree = *m_p_target_index_tree;
    StreamedCacheIndexTreeEntry<Key>& current_node = index_tree[m_current_index];

    if (m_is_at_beginning)
        m_is_at_beginning = false;

    if (current_node.right_leaf)
    {
        // try begin traversing new subtree to the right

        m_current_index = subtree_get_leftmost_node(current_node.right_leaf);
    }
    else if (current_node.inheritance == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child)
    {
        // if there is nothing left to investigate to the right and we live in the left subtree, simply move up

        m_current_index = index_tree[m_current_index].parent_node;
    }
    else
    {
        // we are here only if there was no way to turn to the right and we already living in a right subtree, which means
        // that its direct parent has already been traversed. Move up until we find ourselves in a left subtree or until
        // we reach the root, which identifies the end of tree traversal

        size_t aux{ m_current_index };
        while (index_tree[aux].inheritance == StreamedCacheIndexTreeEntry<Key>::inheritance_category::right_child
            && (aux = index_tree[aux].parent_node));

        if (!aux) m_has_reached_end = true;
        else m_current_index = index_tree[aux].parent_node;
    }

    return *this;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator++(int)
{
    typename StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator rv{ *this };
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
    return const_cast<StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator*>(this)->operator*();
}

template<typename Key, size_t cluster_size>
inline StreamedCacheIndexTreeEntry<Key> const* StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator->() const
{
    return const_cast<StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator*>(this)->operator->();
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
inline typename StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator& StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator--()
{
    if (m_is_at_beginning)
        throw std::out_of_range{ "index tree iterator is out of range" };

    std::vector<StreamedCacheIndexTreeEntry<Key>>& index_tree = *m_p_target_index_tree;
    StreamedCacheIndexTreeEntry<Key>& current_node = index_tree[m_current_index];

    if (m_has_reached_end)
    {
        m_has_reached_end = false;
        return *this;
    }

    if (current_node.left_leaf)
    {
        // try begin traversing subtree to the left
        m_current_index = subtree_get_rightmost_node(current_node.left_leaf);
    }
    else if (current_node.inheritance == StreamedCacheIndexTreeEntry<Key>::inheritance_category::right_child)
    {
        // if find ourselves in right subtree, then the node that was preceding traversal
        // of that subtree was its parent...
        m_current_index = index_tree[m_current_index].parent_node;
    }
    else
    {
        // we end up here only if there was no way to turn to the left (i.e. no nodes preceeding the current
        // node within the current subtree) and we are in left subtree. The move up until we either find
        // ourselves in right subtree again or until we reach the root with the latter identifying the end of tree traversal
        size_t aux{ m_current_index };
        while (index_tree[aux].inheritance == StreamedCacheIndexTreeEntry<Key>::inheritance_category::left_child
            && (aux = index_tree[aux].parent_node));

        if (!aux) m_is_at_beginning = true;
        else m_current_index = index_tree[aux].parent_node;
    }

    return *this;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::operator--(int)
{
    StreamedCacheIndexIterator rv{ *this };
    --(*this);
    return rv;
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::subtree_get_leftmost_node(size_t subtree_root) const
{
    size_t rv{ subtree_root };
    std::vector<StreamedCacheIndexTreeEntry<Key>>& index_tree = *m_p_target_index_tree;

    {
        size_t aux{ 0U };
        while (aux = index_tree[rv].left_leaf) rv = aux;
    }

    return rv;
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexIterator::subtree_get_rightmost_node(size_t subtree_root) const
{
    size_t rv{ subtree_root };
    std::vector<StreamedCacheIndexTreeEntry<Key>>& index_tree = *m_p_target_index_tree;

    {
        size_t aux{ 0U };
        while (aux = index_tree[rv].right_leaf) rv = aux;
    }

    return rv;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator&
StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator::operator++()
{
    ++m_iter;
    return *this;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator
StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator::operator++(int)
{
    StreamedCacheIndexConstIterator rv{ *this };
    ++m_iter;

    return rv;
}

template<typename Key, size_t cluster_size>
inline StreamedCacheIndexTreeEntry<Key> const& StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator::operator*() const
{
    return *m_iter;
}

template<typename Key, size_t cluster_size>
inline StreamedCacheIndexTreeEntry<Key> const* StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator::operator->() const
{
    StreamedCacheIndexTreeEntry<Key> const& aux = *m_iter;
    return &aux;
}

template<typename Key, size_t cluster_size>
inline bool StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator::operator==(StreamedCacheIndexConstIterator const& other) const
{
    return m_iter == other.m_iter;
}

template<typename Key, size_t cluster_size>
inline bool StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator::operator!=(StreamedCacheIndexConstIterator const& other) const
{
    return m_iter != other.m_iter;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator& StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator::operator--()
{
    --m_iter;
    return *this;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator StreamedCacheIndex<Key, cluster_size>::StreamedCacheIndexConstIterator::operator--(int)
{
    StreamedCacheIndexConstIterator rv{ *this };
    --m_iter;

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
    *p_color_inheritance_and_deletion_status = node_color | (static_cast<unsigned char>(inheritance) << 2) | (static_cast<unsigned char>(to_be_deleted) << 4);
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
    inheritance = static_cast<inheritance_category>((*p_color_inheritance_and_deletion_status & 0xC) >> 2);
    to_be_deleted = (*p_color_inheritance_and_deletion_status & 0x10) != 0;
}

template<typename Key, size_t cluster_size>
inline bool StreamedCache<Key, cluster_size>::isCompressed() const
{
    return static_cast<int>(m_compression_level) > 0;
}

template<typename Key, size_t cluster_size>
inline bool StreamedCache<Key, cluster_size>::isGood() const
{
    return m_is_good;
}

template<typename Key, size_t cluster_size>
inline StreamedCache<Key, cluster_size>::operator bool() const
{
    return isGood();
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::pack_date_stamp(misc::DateTime const& date_stamp, unsigned char packed_date_stamp[13])
{
    *reinterpret_cast<uint16_t*>(packed_date_stamp) = date_stamp.year();    // 16-bit storage for year
    uint8_t month = date_stamp.month();
    uint8_t day = date_stamp.day();
    uint8_t hour = date_stamp.hour();
    uint8_t minute = date_stamp.minute();
    double second = date_stamp.second();

    packed_date_stamp[2] = static_cast<unsigned char>(month);    // 4-bit storage for month
    packed_date_stamp[2] |= day << 4;    // 5-bit storage for day
    packed_date_stamp[3] = day >> 4;
    packed_date_stamp[3] |= hour << 1;    // 5-bit storage for hour
    packed_date_stamp[3] |= minute << 6;    // 6-bit storage for minute
    packed_date_stamp[4] = minute >> 2;    // 4-bits reserved for future use (time zones?)

    *reinterpret_cast<double*>(packed_date_stamp + 5) = second;    // 64-bit storage for high-precision second
}

template<typename Key, size_t cluster_size>
inline misc::DateTime StreamedCache<Key, cluster_size>::unpack_date_stamp(unsigned char packed_date_stamp[13])
{
    uint16_t year = *reinterpret_cast<uint16_t*>(packed_date_stamp);
    uint8_t month = packed_date_stamp[2] & 0xF;
    uint8_t day = packed_date_stamp[2] >> 4; day |= (packed_date_stamp[3] & 0x1) << 4;
    uint8_t hour = (packed_date_stamp[3] & 0x3E) >> 1;
    uint8_t minute = packed_date_stamp[3] >> 6; minute |= packed_date_stamp[4] << 2;
    double second = *reinterpret_cast<double*>(packed_date_stamp + 5);

    return misc::DateTime{ year, month, day, hour, minute, second };
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCache<Key, cluster_size>::align_to(size_t value, size_t alignment)
{
    return value + (alignment - value % alignment) % alignment;
}

template<typename Key, size_t cluster_size>
inline std::pair<size_t, bool> StreamedCache<Key, cluster_size>::serialize_entry(
    StreamedCacheEntry<Key, cluster_size> const& entry,
    size_t overwrite_address)
{
    size_t compressed_data_size{ 0U };
    void* p_data_to_serialize{ nullptr };

    if (isCompressed())
    {
        // initialize zlib deflation stream
        z_stream deflation_stream{};

        {
            deflation_stream.zalloc = Z_NULL;
            deflation_stream.zfree = Z_NULL;

            int rv = deflateInit(&deflation_stream, static_cast<int>(m_compression_level));
            if (rv != Z_OK)
            {
                switch (rv)
                {
                case Z_MEM_ERROR:
                    misc::Log::retrieve()->out("Not enough memory to initialize zlib. The streamed cache \"" + getStringName()
                        + "\" will default to uncompressed state", misc::LogMessageType::exclamation);
                    return std::make_pair(0U, false);

                case Z_STREAM_ERROR:
                    misc::Log::retrieve()->out("zlib was provided with incorrect compression level of \"" +
                        std::to_string(static_cast<int>(m_compression_level)) + "\". The streamed cache \"" + getStringName()
                        + "\" will default to uncompressed state", misc::LogMessageType::exclamation);
                    return std::make_pair(0U, false);

                case Z_VERSION_ERROR:
                    misc::Log::retrieve()->out("zlib binary version differs from the version assumed by the caller."
                        " The streamed cache \"" + getStringName()
                        + "\" will default to uncompressed state", misc::LogMessageType::exclamation);
                    return std::make_pair(0U, false);
                }
            }
        }


        // deflate
        {
            deflation_stream.next_in = static_cast<Bytef*>(entry.m_data_blob_to_be_cached.data());
            deflation_stream.avail_in = static_cast<uInt>(entry.m_data_blob_to_be_cached.size());

            uLong deflated_entry_max_size = deflateBound(&deflation_stream, static_cast<uLong>(entry.m_data_blob_to_be_cached.size()));
            if (!m_aux_compression_buffer || m_aux_compression_buffer->size() < deflated_entry_max_size)
                m_aux_compression_buffer.reset(new DataChunk{ 2 * deflated_entry_max_size });

            deflation_stream.next_out = static_cast<Bytef*>(m_aux_compression_buffer->data());
            deflation_stream.avail_out = static_cast<uInt>(m_aux_compression_buffer->size());

            if (deflate(&deflation_stream, Z_FINISH) != Z_STREAM_END)
            {
                misc::Log::retrieve()->out("Unable to compress entry record during serialization to streamed cache \""
                    + getStringName() + "\" (zlib deflate() error", misc::LogMessageType::error);
                return std::make_pair(0U, false);
            }

            if (deflateEnd(&deflation_stream) != Z_OK)
            {
                misc::Log::retrieve()->out("Unable to release zlib deflation stream upon compression finalization of a data chunk",
                    misc::LogMessageType::error);
                return std::make_pair(0U, false);
            }

            compressed_data_size = deflation_stream.total_out;
            p_data_to_serialize = m_aux_compression_buffer->data();
        }
    }
    else
    {
        compressed_data_size = entry.m_data_blob_to_be_cached.size();
        p_data_to_serialize = entry.m_data_blob_to_be_cached.data();
    }


    size_t new_entry_size = m_entry_record_overhead + compressed_data_size;
    std::pair<size_t, size_t> cache_allocation_desc{};    // where to write data eventually
    if (overwrite_address)
    {
        // the entry must overwrite already existing entry

        size_t existing_entry_sequence_length{ 0U };
        m_cache_stream.seekg(overwrite_address, std::ios::beg);
        m_cache_stream.read(reinterpret_cast<char*>(&existing_entry_sequence_length), m_sequence_overhead);
        size_t existing_entry_cluster_aligned_size{ existing_entry_sequence_length * cluster_size };

        if (existing_entry_cluster_aligned_size < new_entry_size + m_sequence_overhead)
        {
            // need to allocate more space for new data

            size_t extra_space_requirement{
                static_cast<size_t>((std::max)(static_cast<long long>(new_entry_size) - static_cast<long long>(existing_entry_cluster_aligned_size), 0ll))
            };

            std::pair<size_t, size_t> existing_entry_sequence_allocation_desc = std::make_pair(overwrite_address, existing_entry_sequence_length);
            std::pair<size_t, size_t> extra_space_allocation_desc = allocate_space_in_cache(extra_space_requirement);
            if (!extra_space_allocation_desc.second) return std::make_pair(0U, false);

            std::list<std::pair<size_t, size_t>> reserved_seq_list{ existing_entry_sequence_allocation_desc, extra_space_allocation_desc };
            cache_allocation_desc = optimize_reservation(reserved_seq_list, extra_space_requirement);
        }
        else
        {

            // possibly, need to free up some space that remains unused after new data is written

            size_t redundant_sequence_length = (existing_entry_cluster_aligned_size - (new_entry_size + m_sequence_overhead)) / cluster_size;
            size_t contracted_sequence_length = existing_entry_sequence_length - redundant_sequence_length;
            if (redundant_sequence_length)
            {
                size_t redundant_sequence_base_address =
                    get_cluster_base_address(std::make_pair(overwrite_address, existing_entry_sequence_length), contracted_sequence_length);

                m_cache_stream.seekp(redundant_sequence_base_address, std::ios::beg);
                size_t aux{ redundant_sequence_length }; m_cache_stream.write(reinterpret_cast<char*>(&redundant_sequence_length), m_sequence_overhead);
                m_empty_cluster_table.push_back(redundant_sequence_base_address);

                m_cache_stream.seekp(overwrite_address, std::ios::beg);
                m_cache_stream.write(reinterpret_cast<char*>(&contracted_sequence_length), m_sequence_overhead);
            }

            cache_allocation_desc.first = overwrite_address;
            cache_allocation_desc.second = contracted_sequence_length;
        }
    }
    else
    {
        // the entry is new

        cache_allocation_desc = allocate_space_in_cache(new_entry_size);
        if (!cache_allocation_desc.second) return std::make_pair(0U, false);
    }


    {
        // write data into the cache stream

        m_cache_stream.seekp(cache_allocation_desc.first + m_sequence_overhead, std::ios::beg);

        unsigned char packed_date_stamp[m_datestamp_size];
        pack_date_stamp(entry.m_date_stamp, packed_date_stamp);
        m_cache_stream.write(reinterpret_cast<char*>(packed_date_stamp), m_datestamp_size);

        size_t uncompressed_entry_size{ entry.m_data_blob_to_be_cached.size() };
        m_cache_stream.write(reinterpret_cast<char*>(&uncompressed_entry_size), m_uncompressed_size_record);

        uint64_t current_cluster_base_address{ cache_allocation_desc.first + m_sequence_overhead + m_entry_record_overhead };
        size_t total_bytes_left_to_write = compressed_data_size;
        size_t total_bytes_written{ 0U };
        size_t num_bytes_to_write_into_current_cluster{
            (std::min)(total_bytes_left_to_write,
            cluster_size - m_sequence_overhead - m_entry_record_overhead) };
        while (total_bytes_left_to_write)
        {
            m_cache_stream.seekp(current_cluster_base_address, std::ios::beg);
            m_cache_stream.write(static_cast<char*>(p_data_to_serialize) + total_bytes_written,
                num_bytes_to_write_into_current_cluster);

            m_cache_stream.seekg(m_cache_stream.tellp());
            m_cache_stream.read(reinterpret_cast<char*>(&current_cluster_base_address), 8U);

            total_bytes_left_to_write -= num_bytes_to_write_into_current_cluster;
            total_bytes_written += num_bytes_to_write_into_current_cluster;
            num_bytes_to_write_into_current_cluster = (std::min)(total_bytes_left_to_write, cluster_size);
        }
    }


    return std::make_pair(cache_allocation_desc.first, true);
}

template<typename Key, size_t cluster_size>
inline std::pair<SharedDataChunk, size_t> StreamedCache<Key, cluster_size>::deserialize_entry(size_t data_offset) const
{
    m_cache_stream.seekg(data_offset, std::ios::beg);
    uint64_t sequence_length; m_cache_stream.read(reinterpret_cast<char*>(&sequence_length), 8U);
    m_cache_stream.seekg(m_datestamp_size, std::ios::cur);    // skip the datestamp
    uint64_t uncompressed_entry_size; m_cache_stream.read(reinterpret_cast<char*>(&uncompressed_entry_size), 8U);

    SharedDataChunk output_data_chunk{ sequence_length * cluster_size };
    char* p_data = static_cast<char*>(output_data_chunk.data());
    uint64_t cluster_base_offset{ data_offset + m_sequence_overhead + m_entry_record_overhead };
    size_t reading_offset{ 0U };
    for (uint64_t i = 0; i < sequence_length; ++i)
    {
        m_cache_stream.seekg(cluster_base_offset, std::ios::beg);
        size_t num_bytes_to_read{ i == 0
            ? cluster_size - m_sequence_overhead - m_entry_record_overhead
            : cluster_size };
        m_cache_stream.read(p_data + reading_offset, num_bytes_to_read);
        reading_offset += num_bytes_to_read;
        m_cache_stream.read(reinterpret_cast<char*>(&cluster_base_offset), 8U);
    }

    return std::make_pair(output_data_chunk, static_cast<size_t>(uncompressed_entry_size));
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::write_header_data()
{
    m_cache_stream.seekp(0, std::ios::beg);
    m_cache_stream.write(m_magic_bytes, sizeof(m_magic_bytes));
    m_cache_stream.write(reinterpret_cast<char*>(const_cast<uint32_t*>(&m_version)), 4U);

    union {
        uint32_t flag;
        char bytes[4];
    }endianness;
    endianness.flag = 0x01020304;
    m_cache_stream.write(endianness.bytes, 4U);

    uint64_t aux;

    aux = m_max_cache_size; m_cache_stream.write(reinterpret_cast<char*>(&aux), 8U);
    aux = m_cache_body_size; m_cache_stream.write(reinterpret_cast<char*>(&aux), 8U);

    uint64_t size_of_index_tree = m_index.getSize();
    m_cache_stream.write(reinterpret_cast<char*>(&size_of_index_tree), 8U);

    aux = m_index.m_max_index_redundant_growth_pressure; m_cache_stream.write(reinterpret_cast<char*>(&aux), 8U);
    aux = m_index.m_current_index_redundant_growth_pressure; m_cache_stream.write(reinterpret_cast<char*>(&aux), 8U);

    uint64_t size_of_empty_cluster_table = m_empty_cluster_table.size() * m_eclt_entry_size;
    m_cache_stream.write(reinterpret_cast<char*>(&size_of_empty_cluster_table), 8U);

    char flags = static_cast<char>(m_compression_level) & 0xF | static_cast<char>(m_are_overwrites_allowed) << 4;
    m_cache_stream.write(&flags, 1U);
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::write_index_data()
{
    m_cache_stream.seekp(m_header_size + m_cache_body_size, std::ios::beg);
    DataChunk index_tree_entry_serialization_chunk{ StreamedCacheIndexTreeEntry<Key>::serialized_size };
    for (size_t i = 0; i < m_index.m_index_tree.size(); ++i)
    {
        StreamedCacheIndexTreeEntry<Key>& entry = m_index.m_index_tree[i];
        entry.prepare_serialization_blob(index_tree_entry_serialization_chunk.data());
        m_cache_stream.write(static_cast<char*>(index_tree_entry_serialization_chunk.data()),
            index_tree_entry_serialization_chunk.size());
    }
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::write_eclt_data()
{
    m_cache_stream.seekp(m_header_size + m_cache_body_size + m_index.getSize(), std::ios::beg);
    std::vector<uint64_t> eclt(m_empty_cluster_table.size());
    std::transform(m_empty_cluster_table.begin(), m_empty_cluster_table.end(), eclt.begin(),
        [](size_t e) -> uint64_t
        {
            return static_cast<uint64_t>(e);
        });
    m_cache_stream.write(reinterpret_cast<char*>(eclt.data()), 8U * eclt.size());
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::load_service_data()
{
    // retrieve the version of the streamed cache
    {
        m_cache_stream.seekg(0, std::ios::beg);

        char magic_bytes[sizeof(m_magic_bytes)] = {};
        m_cache_stream.read(magic_bytes, sizeof(m_magic_bytes));
        if (memcmp(magic_bytes, m_magic_bytes, sizeof(m_magic_bytes)))
        {
            misc::Log::retrieve()->out("Streamed cache \"" + getStringName() + "\" "
                "cannot be loaded as it has invalid format or has been corrupted",
                misc::LogMessageType::error);
            m_is_good = false;
            return;
        }


        uint32_t cache_version;
        m_cache_stream.read(reinterpret_cast<char*>(&cache_version), 4U);

        uint16_t assumed_major = m_version >> 16;
        uint16_t assumed_minor = m_version & 0xFFFF;
        uint16_t loaded_major = cache_version >> 16;
        uint16_t loaded_minor = cache_version & 0xFFFF;

        if (assumed_major < loaded_major || assumed_major == loaded_major && assumed_minor < loaded_minor)
        {
            misc::Log::retrieve()->out("Streamed cache \"" + getStringName() + "\" cannot be loaded as it's version is "
                + std::to_string(loaded_major) + "." + std::to_string(loaded_minor) + " whereas the highest version supported by "
                "the parser is " + std::to_string(assumed_major) + "." + std::to_string(assumed_minor), misc::LogMessageType::error);

            m_is_good = false;
            return;
        }
    }

    // retrieve endianness of the streamed cache and compare it with the endianness of the host
    {
        union {
            uint32_t flag;
            char bytes[4];
        }this_machine_endianness, serialization_machine_endianness;
        this_machine_endianness.flag = 0x01020304;
        m_cache_stream.read(serialization_machine_endianness.bytes, 4U);
        m_endianness_conversion_required = memcmp(this_machine_endianness.bytes, serialization_machine_endianness.bytes, 4U) != 0;
    }

    // retrieve parameters of the cache body
    {
        uint64_t aux;
        m_cache_stream.read(reinterpret_cast<char*>(&aux), 8U); m_max_cache_size = aux;
        m_cache_stream.read(reinterpret_cast<char*>(&aux), 8U); m_cache_body_size = aux;
    }

    // retrieve parameters of the index tree
    uint64_t size_of_index_tree;
    {
        m_cache_stream.read(reinterpret_cast<char*>(&size_of_index_tree), 8U);

        uint64_t aux;
        m_cache_stream.read(reinterpret_cast<char*>(&aux), 8U); m_index.m_max_index_redundant_growth_pressure = aux;
        m_cache_stream.read(reinterpret_cast<char*>(&aux), 8U); m_index.m_current_index_redundant_growth_pressure = aux;
    }

    // retrieve size of the ECLT
    uint64_t size_of_empty_cluster_table;
    {
        m_cache_stream.read(reinterpret_cast<char*>(&size_of_empty_cluster_table), 8U);
    }

    // parse flags of the streamed cache
    {
        char flags;
        m_cache_stream.read(&flags, 1U);
        m_compression_level = static_cast<StreamedCacheCompressionLevel>(flags & 0xF);
        m_are_overwrites_allowed = (flags >> 4 & 0x1) != 0;
    }

    load_index_data(size_of_index_tree);
    load_eclt_data(size_of_empty_cluster_table);
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::load_index_data(size_t index_tree_size_in_bytes)
{
    m_cache_stream.seekg(m_header_size + m_cache_body_size, std::ios::beg);
    size_t num_entries_in_index_tree = index_tree_size_in_bytes / StreamedCacheIndexTreeEntry<Key>::serialized_size;
    DataChunk index_data_blob{ StreamedCacheIndexTreeEntry<Key>::serialized_size };
    size_t num_alive_entries{ 0U };
    for (size_t i = 0; i < num_entries_in_index_tree; ++i)
    {
        m_cache_stream.read(static_cast<char*>(index_data_blob.data()), index_data_blob.size());
        StreamedCacheIndexTreeEntry<Key> e{};
        e.deserialize_from_blob(index_data_blob.data());
        m_index.m_index_tree.push_back(e);
        if (!e.to_be_deleted) ++num_alive_entries;
    }
    m_index.m_number_of_entries = num_alive_entries;
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::load_eclt_data(size_t eclt_data_size_in_bytes)
{
    m_cache_stream.seekg(m_header_size + m_cache_body_size + m_index.getSize(), std::ios::beg);
    size_t num_entries_in_eclt = eclt_data_size_in_bytes / 8U;

    std::vector<uint64_t> aux_vector(num_entries_in_eclt);
    m_cache_stream.read(reinterpret_cast<char*>(aux_vector.data()), eclt_data_size_in_bytes);
    m_empty_cluster_table.resize(num_entries_in_eclt);
    std::transform(aux_vector.begin(), aux_vector.end(), m_empty_cluster_table.begin(),
        [](uint64_t e) -> size_t
        {
            return static_cast<size_t>(e);
        });
}

template<typename Key, size_t cluster_size>
inline std::pair<size_t, size_t> StreamedCache<Key, cluster_size>::reserve_available_cluster_sequence(size_t size_hint)
{
    if (m_empty_cluster_table.size())
    {
        size_t base_offset = m_empty_cluster_table.back(); m_empty_cluster_table.pop_back();
        m_cache_stream.seekg(base_offset, std::ios::beg);

        uint64_t cluster_sequence_length;
        m_cache_stream.read(reinterpret_cast<char*>(&cluster_sequence_length), 8U);

        return std::make_pair(base_offset, static_cast<size_t>(cluster_sequence_length));
    }


    size_t num_unpartitioned_clusters = (m_max_cache_size - m_cache_body_size) / (cluster_size + m_cluster_overhead);
    if (!num_unpartitioned_clusters)
        return std::make_pair<size_t, size_t>(0U, 0U);

    size_t new_sequence_base_offset = m_header_size + m_cache_body_size;
    uint64_t new_sequence_length = (std::min)(
        num_unpartitioned_clusters,
        align_to(size_hint, cluster_size) / cluster_size);
    size_t new_sequence_real_capacity = new_sequence_length * (cluster_size + m_cluster_overhead);

    // If new sequence "eats" some extra space due to unused portion of the last cluster, we still guarantee that
    // capacity requested upon creation of the cache will not degrade by extending its maximal size by an 
    // extra cluster (plus its overhead)
    if (new_sequence_real_capacity > size_hint)
        m_max_cache_size += cluster_size + m_cluster_overhead;

    m_cache_stream.seekp(new_sequence_base_offset, std::ios::beg);
    m_cache_stream.write(reinterpret_cast<char*>(&new_sequence_length), 8U);

    uint64_t cluster_base_offset{ new_sequence_base_offset };
    for (size_t i = 0; i < new_sequence_length - 1; ++i, cluster_base_offset += cluster_size + m_cluster_overhead)
    {
        uint64_t next_cluster_base_offset = cluster_base_offset + cluster_size + m_cluster_overhead;
        m_cache_stream.seekp(cluster_base_offset + cluster_size, std::ios::beg);
        m_cache_stream.write(reinterpret_cast<char*>(&next_cluster_base_offset), 8U);
    }
    m_cache_body_size += new_sequence_real_capacity;

    return std::make_pair(new_sequence_base_offset, static_cast<size_t>(new_sequence_length));
}

template<typename Key, size_t cluster_size>
inline std::pair<size_t, size_t> StreamedCache<Key, cluster_size>::allocate_space_in_cache(size_t size)
{
    std::list<std::pair<size_t, size_t>> reserved_sequence_list{};
    size_t allocated_so_far{ 0U };
    size_t requested_capacity_plus_overhead = size + m_sequence_overhead;

    size_t max_allocation_size = align_to(requested_capacity_plus_overhead, cluster_size) / cluster_size * (cluster_size + m_cluster_overhead);
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
        else
        {
            allocated_so_far += cluster_sequence_desc.second * cluster_size;
            reserved_sequence_list.push_back(cluster_sequence_desc);
        }
    }

    return reserved_sequence_list.size()
        ? optimize_reservation(reserved_sequence_list, requested_capacity_plus_overhead)
        : std::make_pair<size_t, size_t>(0U, 0U);
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCache<Key, cluster_size>::get_cluster_base_address(
    std::pair<size_t, size_t> const& sequence_allocation_desc,
    size_t cluster_idx)
{
    assert(cluster_idx < sequence_allocation_desc.second);

    size_t current_cluster_offset{ sequence_allocation_desc.first };
    for (size_t i = 0U; i < cluster_idx; ++i)
    {
        m_cache_stream.seekg(current_cluster_offset + cluster_size, std::ios::beg);
        m_cache_stream.read(reinterpret_cast<char*>(&current_cluster_offset), m_cluster_overhead);
    }
    return current_cluster_offset;
}

template<typename Key, size_t cluster_size>
inline std::pair<size_t, size_t> StreamedCache<Key, cluster_size>::optimize_reservation(
    std::list<std::pair<size_t, size_t>>& reserved_sequence_list,
    size_t size_hint)
{
    uint64_t total_sequence_length{ 0U };
    for (auto p = reserved_sequence_list.begin(); p != reserved_sequence_list.end(); ++p)
    {
        std::list<std::pair<size_t, size_t>>::iterator q{ p }; ++q;
        uint64_t next_sequence_base_address = q != reserved_sequence_list.end() ? q->first : 0U;

        m_cache_stream.seekp(get_cluster_base_address(*p, p->second - 1) + cluster_size, std::ios::beg);
        m_cache_stream.write(reinterpret_cast<char*>(&next_sequence_base_address), 8U);

        total_sequence_length += p->second;
    }

    // remove redundant clusters if possible
    size_t total_sequence_capacity = total_sequence_length * cluster_size;
    if (total_sequence_capacity > size_hint)
    {
        size_t redundant_space = total_sequence_capacity - size_hint;
        size_t redundant_sequence_length = redundant_space / cluster_size;

        if (redundant_sequence_length)
        {
            auto& last_sequence_desc = reserved_sequence_list.back();
            size_t contracted_sequence_length = last_sequence_desc.second - redundant_sequence_length;
            size_t dissected_sequence_base_address = get_cluster_base_address(last_sequence_desc, contracted_sequence_length);

            m_cache_stream.seekp(dissected_sequence_base_address, std::ios::beg);
            size_t aux{ redundant_sequence_length }; m_cache_stream.write(reinterpret_cast<char*>(&aux), 8U);
            m_empty_cluster_table.push_back(dissected_sequence_base_address);

            total_sequence_length -= redundant_sequence_length;
        }
    }

    m_cache_stream.seekp(reserved_sequence_list.front().first, std::ios::beg);
    m_cache_stream.write(reinterpret_cast<char*>(&total_sequence_length), 8U);

    return std::make_pair(reserved_sequence_list.front().first, static_cast<size_t>(total_sequence_length));
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::remove_oldest_entry_record()
{
    misc::DateTime oldest_entry_datestampt{ 0xFFFF, 12, 31, 23, 59, 59.9999 };
    StreamedCacheIndexTreeEntry<Key>* p_oldest_entry{ nullptr };
    for (StreamedCacheIndexTreeEntry<Key>& e : m_index)
    {
        m_cache_stream.seekg(e.data_offset + m_sequence_overhead, std::ios::beg);
        unsigned char packed_date_stamp[13];
        m_cache_stream.read(reinterpret_cast<char*>(packed_date_stamp), 13);
        misc::DateTime unpacked_date_stamp = unpack_date_stamp(packed_date_stamp);
        if (unpacked_date_stamp <= oldest_entry_datestampt)
        {
            oldest_entry_datestampt = unpacked_date_stamp;
            p_oldest_entry = &e;
        }
    }

    m_index.remove_entry(p_oldest_entry->cache_entry_key);
    m_empty_cluster_table.push_back(static_cast<size_t>(p_oldest_entry->data_offset));
}

template<typename Key, size_t cluster_size>
inline StreamedCache<Key, cluster_size>::StreamedCache(std::iostream& cache_io_stream, size_t capacity,
    StreamedCacheCompressionLevel compression_level/* = StreamCacheCompressionLevel::level0*/, bool are_overwrites_allowed/* = false*/) :
    m_cache_stream{ cache_io_stream },
    m_max_cache_size{
    align_to(
        align_to(capacity, cluster_size) / cluster_size * (cluster_size + m_cluster_overhead + m_sequence_overhead + m_entry_record_overhead),
        cluster_size + m_cluster_overhead) },
    m_cache_body_size{ 0U },
    m_compression_level{ compression_level },
    m_are_overwrites_allowed{ are_overwrites_allowed },
    m_is_finalized{ false },
    m_is_read_only{ false },
    m_is_good{ true }
{
    if (!cache_io_stream)
    {
        misc::Log::retrieve()->out("Unable to open cache IO stream", misc::LogMessageType::error);
        m_is_good = false;
        return;
    }
}



template<typename Key, size_t cluster_size>
inline StreamedCache<Key, cluster_size>::StreamedCache(std::iostream& cache_io_stream, bool read_only/* = false*/) :
    m_cache_stream{ cache_io_stream },
    m_is_finalized{ false },
    m_is_read_only{ read_only },
    m_is_good{ true }
{
    if (!cache_io_stream)
    {
        misc::Log::retrieve()->out("Unable to open cache IO stream", misc::LogMessageType::error);
        m_is_good = false;
        return;
    }

    load_service_data();
}

template<typename Key, size_t cluster_size>
inline StreamedCache<Key, cluster_size>::StreamedCache(StreamedCache&& other) :
    m_cache_stream{ other.m_cache_stream },
    m_max_cache_size{ other.m_max_cache_size },
    m_cache_body_size{ other.m_cache_body_size },
    m_index{ std::move(other.m_index) },
    m_empty_cluster_table{ std::move(other.m_empty_cluster_table) },
    m_compression_level{ std::move(other.m_compression_level) },
    m_are_overwrites_allowed{ other.m_are_overwrites_allowed },
    m_endianness_conversion_required{ other.m_endianness_conversion_required },
    m_is_finalized{ other.m_is_finalized },
    m_aux_compression_buffer{ std::move(other.m_aux_compression_buffer) },
    m_is_read_only{ other.m_is_read_only },
    m_is_good{ other.m_is_good }
{
    other.m_is_finalized = true;
}

template<typename Key, size_t cluster_size>
inline StreamedCache<Key, cluster_size>::~StreamedCache()
{
    if (!m_is_finalized && m_is_good) finalize();
}

template<typename Key, size_t cluster_size>
inline bool StreamedCache<Key, cluster_size>::addEntry(entry_type const& entry, bool force_overwrite /* = false*/)
{
    if (m_is_finalized || m_is_read_only) return false;

    auto entry_base_offset = m_index.get_cache_entry_data_offset_from_key(entry.m_key);
    size_t overwrite_offset{ 0U };    // 0 means not to overwrite anything

    if (entry_base_offset.isValid())
    {
        if (!force_overwrite && !m_are_overwrites_allowed)
        {
            misc::Log::retrieve()->out("Unable to serialize entry to stream cache \"" + getStringName() + "\"."
                "Entry with provided key value \"" + entry.m_key.toString() + "\" already exists", misc::LogMessageType::error);
            return false;
        }
        else
        {
            overwrite_offset = static_cast<size_t>(static_cast<uint64_t>(entry_base_offset));
        }
    }


    std::pair<size_t, bool> rv = serialize_entry(entry, overwrite_offset);
    if (!rv.second)
    {
        misc::Log::retrieve()->out("Error while serializing entry into stream cache \"" + getStringName() + "\"", misc::LogMessageType::error);
        return false;
    }
    else if (!overwrite_offset)
    {
        m_index.add_entry(std::make_pair(entry.m_key, rv.first));
    }

    return true;
}

template<typename Key, size_t cluster_size>
inline void StreamedCache<Key, cluster_size>::finalize()
{
    assert(!m_is_finalized);

    if (!m_is_read_only)
    {
        write_header_data();
        write_index_data();
        write_eclt_data();
        m_cache_stream.flush();
    }

    m_is_finalized = true;
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCache<Key, cluster_size>::freeSpace() const
{
    return m_max_cache_size - usedSpace();
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCache<Key, cluster_size>::usedSpace() const
{
    size_t emptied_cluster_sequences_total_capacity{ 0U };
    for (size_t addr : m_empty_cluster_table)
    {
        m_cache_stream.seekg(addr, std::ios::beg);
        uint64_t cluster_sequence_length;
        m_cache_stream.read(reinterpret_cast<char*>(&cluster_sequence_length), 8U);
        emptied_cluster_sequences_total_capacity += static_cast<size_t>(cluster_sequence_length) * cluster_size;
    }

    size_t unpartitioned_cache_size = m_max_cache_size - m_cache_body_size;
    size_t cluster_reservation_overhead = unpartitioned_cache_size / (cluster_size + m_cluster_overhead) * m_cluster_overhead;
    return (std::min)(m_max_cache_size,
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
inline SharedDataChunk StreamedCache<Key, cluster_size>::retrieveEntry(Key const& entry_key, size_t* valid_bytes_count) const
{
    auto rv = m_index.get_cache_entry_data_offset_from_key(entry_key);
    if (!rv.isValid())
    {
        misc::Log::retrieve()->out("Unable to retrieve entry with key \""
            + entry_key.toString() + "\" from streamed cache \""
            + getStringName() + "\".", misc::LogMessageType::error);
        return SharedDataChunk{};
    }

    std::pair<SharedDataChunk, size_t> raw_data_chunk_and_uncompressed_size = deserialize_entry(rv);
    if (static_cast<int>(m_compression_level) > 0)
    {
        // the data are compressed and should be inflated before getting returned to the caller
        z_stream zlib_deflation_stream;
        zlib_deflation_stream.next_in = static_cast<Bytef*>(raw_data_chunk_and_uncompressed_size.first.data());
        zlib_deflation_stream.avail_in = static_cast<uInt>(raw_data_chunk_and_uncompressed_size.first.size());
        zlib_deflation_stream.zalloc = Z_NULL;
        zlib_deflation_stream.zfree = Z_NULL;

        int rv = inflateInit(&zlib_deflation_stream);
        if (rv != Z_OK)
        {
            switch (rv)
            {
            case Z_MEM_ERROR:
                misc::Log::retrieve()->out("Unable to inflate entry with key \""
                    + entry_key.toString() + "\" from streamed cache \""
                    + getStringName() + "\"; zlib is out of memory",
                    misc::LogMessageType::error);
                break;

            case Z_VERSION_ERROR:
                misc::Log::retrieve()->out("Unable to inflate entry with key \""
                    + entry_key.toString() + "\" from streamed cache \""
                    + getStringName() + "\"; zlib version assumed by the streamed cache differs from "
                    "the source actually linked to the executable",
                    misc::LogMessageType::error);
                break;

            case Z_STREAM_ERROR:
                misc::Log::retrieve()->out("Unable to inflate entry with key \""
                    + entry_key.toString() + "\" from streamed cache \""
                    + getStringName() + "\"; zlib's inflateInit() function was supplied with invalid parameters",
                    misc::LogMessageType::error);
                break;
            }

            return SharedDataChunk{};
        }

        SharedDataChunk uncompressed_data_chunk{ raw_data_chunk_and_uncompressed_size.second };
        zlib_deflation_stream.next_out = static_cast<Bytef*>(uncompressed_data_chunk.data());
        zlib_deflation_stream.avail_out = static_cast<uInt>(uncompressed_data_chunk.size());
        if (inflate(&zlib_deflation_stream, Z_FINISH) != Z_STREAM_END)
        {
            misc::Log::retrieve()->out("Unable to inflate entry with key \""
                + entry_key.toString() + "\" from streamed cache \""
                + getStringName() + "\"", misc::LogMessageType::error);
            return SharedDataChunk{};
        }

        if (inflateEnd(&zlib_deflation_stream) != Z_OK)
        {
            misc::Log::retrieve()->out("Unable to finalize zlib stream after inflating entry with key \""
                + entry_key.toString() + "\" from streamed cache \""
                + getStringName() + "\"", misc::LogMessageType::error);
            return SharedDataChunk{};
        }

        if(valid_bytes_count) *valid_bytes_count = zlib_deflation_stream.total_out;
        return uncompressed_data_chunk;
    }
    else
    {
        if(valid_bytes_count) *valid_bytes_count = raw_data_chunk_and_uncompressed_size.second;
        return raw_data_chunk_and_uncompressed_size.first;
    }
}

template<typename Key, size_t cluster_size>
inline misc::DateTime StreamedCache<Key, cluster_size>::getEntryTimestamp(Key const& entry_key) const
{
    auto entry_base_offset = m_index.get_cache_entry_data_offset_from_key(entry_key);

    if (!entry_base_offset.isValid())
    {
        misc::Log::retrieve()->out("Unable to retrieve time stamp of entry with key \""
            + entry_key.toString() + "\" from stream cache \"" + getStringName() + "\": "
            "requested entry does not exist in the cache", misc::LogMessageType::exclamation);

        return misc::DateTime::now();
    }

    m_cache_stream.seekg(static_cast<uint64_t>(entry_base_offset) + m_sequence_overhead, std::ios::beg);
    char packed_datestamp_data[m_datestamp_size];
    m_cache_stream.read(packed_datestamp_data, m_datestamp_size);
    return unpack_date_stamp(reinterpret_cast<unsigned char*>(packed_datestamp_data));
}

template<typename Key, size_t cluster_size>
inline size_t StreamedCache<Key, cluster_size>::getEntrySize(Key const& entry_key) const
{
    auto entry_base_offset = m_index.get_cache_entry_data_offset_from_key(entry_key);

    if (!entry_base_offset.isValid())
    {
        misc::Log::retrieve()->out("Unable to retrieve uncompressed size of entry with key \""
            + entry_key.toString() + "\" from stream cache \"" + getStringName() + "\": "
            "requested entry cannot be located in the cache", misc::LogMessageType::exclamation);

        return static_cast<size_t>(-1);
    }

    m_cache_stream.seekg(static_cast<uint64_t>(entry_base_offset) + m_sequence_overhead + m_datestamp_size, std::ios::beg);
    size_t uncompressed_entry_size{ 0U };
    m_cache_stream.read(reinterpret_cast<char*>(&uncompressed_entry_size), m_uncompressed_size_record);
    return uncompressed_entry_size;
}

template<typename Key, size_t cluster_size>
inline bool StreamedCache<Key, cluster_size>::removeEntry(Key const& entry_key)
{
    if (m_is_read_only || m_is_finalized) return false;

    auto rv = m_index.get_cache_entry_data_offset_from_key(entry_key);
    if (!rv.isValid())
    {
        misc::Log::retrieve()->out("Unable to remove entry with key \"" + entry_key.toString() + "\" from streamed cache \""
            + getStringName() + "\". The entry with requested key is not found in the cache", misc::LogMessageType::error);
        return false;
    }

    size_t base_offset = rv;
    m_empty_cluster_table.push_back(base_offset);
    m_index.remove_entry(entry_key);

    return true;
}

template<typename Key, size_t cluster_size>
inline bool StreamedCache<Key, cluster_size>::doesEntryExist(Key const& entry_key) const
{
    return m_index.bst_search(entry_key).second;
}

template<typename Key, size_t cluster_size>
inline StreamedCacheIndex<Key, cluster_size> const& StreamedCache<Key, cluster_size>::getIndex() const
{
    return m_index;
}

template<typename Key, size_t cluster_size>
inline std::pair<uint16_t, uint16_t> StreamedCache<Key, cluster_size>::getVersion() const
{
    return std::make_pair<uint16_t, uint16_t>(m_version >> 16, m_version & 0xFFFF);
}

template<typename Key, size_t cluster_size>
inline bool StreamedCache<Key, cluster_size>::writeCustomHeader(CustomHeader const& custom_header)
{
    if (m_is_read_only || m_is_finalized) return false;

    m_cache_stream.seekp(m_header_size - CustomHeader::size, std::ios::beg);
    m_cache_stream.write(custom_header.data, CustomHeader::size);

    return true;
}

template<typename Key, size_t cluster_size>
inline typename StreamedCache<Key, cluster_size>::CustomHeader StreamedCache<Key, cluster_size>::retrieveCustomHeader() const
{
    m_cache_stream.seekg(m_header_size - CustomHeader::size, std::ios::beg);
    CustomHeader rv{};
    m_cache_stream.read(rv.data, CustomHeader::size);
    return rv;
}

template<typename Key, size_t cluster_size>
template<typename T1, typename T2>
inline void core::StreamedCacheIndex<Key, cluster_size>::swap_values(T1& value1, T2& value2)
{
    value1 = value1 ^ value2;
    value2 = value2 ^ value1;
    value1 = value1 ^ value2;
}

}
#endif
