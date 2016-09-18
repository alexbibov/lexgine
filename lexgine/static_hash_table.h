#ifndef LEXGINE_CORE_MISC_STATIC_HASH_TABLE_H
#define LEXGINE_CORE_MISC_STATIC_HASH_TABLE_H

//NOTE: this implementation is curious, but as such is an overkill. Similar benefits can be achieved by means of the standard functionality of the language

#include <utility>

namespace lexgine { namespace core { namespace misc {

//! Implements static hash table constructed during compile time with ability to contain generic data types.
//! Here @param V is the type of the values stored in the hash table,
//! @param Hash<H> is a template object implementing static function hash<typename KVType>() that takes
//! custom key-value type and returns hash value of type @param H
template<typename H, template<typename H> class Hash, typename ... KVPairs> class StaticHashTable;



// auxiliary type that helps to convert template parameter pack of chars to a C-string
template<char ... chars> struct string_assembler_helper;

template<char chr0, char ... chars>
struct string_assembler_helper<chr0, chars...>
{
    static constexpr char c_str[] = { chr0, chars... };
    static size_t const length = 1U + string_assembler_helper<chars...>::length;
};

template<>
struct string_assembler_helper<'\0'>
{
    static size_t const length = 0U;
};

template<char ... chars>
using string_assembler = string_assembler_helper<chars..., '\0'>;



// auxiliary type encapsulating key-value pair with string key
// note that V must be a literal type
template<typename V, typename KeyStringEncapsulator, char ... key_chars>
class KeyValuePair
{
public:
    using value_type = V;    //!< alias of the type of value
    using key_type = const char*;   //!< alias of the key type
    using key_string_encapsulator = KeyStringEncapsulator;  //!< alias of the type encapsulating constexpr containing the key string

    static constexpr char const* key = string_assembler<key_chars...>::c_str;
    static const size_t key_length = string_assembler<key_chars...>::length;

    KeyValuePair(V const& value) : m_value{ value }
    {

    }

    KeyValuePair(V&& value) : m_value{ std::move(value) }
    {

    }

    V const& value() const { return m_value; }
    V& value() { return m_value; }


    KeyValuePair() = default;   // for some reason MSVCv14 does not generate the default constructor automatically for this type. Even though it should.

private:
    V m_value;
};



// helper type that is used to determine length of a C-string at compile time
template<typename StringEncapsulator>
class StringLengthCalculator
{
private:
    static constexpr size_t c_str_len(char const c_str[])
    {
        return *c_str ? 1 + c_str_len(c_str + 1) : 0;
    }

public:
    static constexpr const size_t result = c_str_len(StringEncapsulator::str());
};



// helper type that simplifies creation of KeyValuePair types
template<typename V, typename StringEncapsulator> class KeyValuePairCreator
{
private:
    static constexpr size_t key_length = StringLengthCalculator<StringEncapsulator>::result;

    template<typename V, size_t len, char ... chars>
    struct aux_key_value_encapsulator
    {
        using result = typename aux_key_value_encapsulator<V, len - 1, StringEncapsulator::str()[len - 1], chars...>::result;
    };

    template<typename V, char ... chars>
    struct aux_key_value_encapsulator<V, 0, chars...>
    {
        using result = KeyValuePair<V, StringEncapsulator, chars...>;
    };

public:
    using result = typename aux_key_value_encapsulator<V, key_length>::result;
};



// helper: allows to compare strings at compile time. Note that template parameter only accepts pointers with non-local linkage (which is always the case
// in our implementation since the pointer is taken from a constexpr character array
template<typename StringEncapsulator1, typename StringEncapsulator2>
class StringComparator
{
private:
    static constexpr bool compare(char const str1[], char const str2[])
    {
        return !str1[0] && !str2[0] ? true : str1[0] == str2[0] && compare(str1 + 1, str2 + 1);
    }

public:
    static constexpr bool is_equal()
    {
        return StringLengthCalculator<StringEncapsulator1>::result == StringLengthCalculator<StringEncapsulator2>::result
            && compare(StringEncapsulator1::str(), StringEncapsulator2::str());
    }
};



// auxiliary type representing static hash bucket
template<typename ... KVTypes> class HashBucket;



template<> class HashBucket<>   // partial specialization for an empty hash bucket
{
public:
    template<typename KVType>
    using add_entry_type = HashBucket<KVType>;  //!< type of the hash bucket with a new entry added

    static size_t const size = 0;   //!< number of values stored in the datum

    //! Always returns 'false' for empty bucket
    template<typename KVType>
    static constexpr bool hasKey()
    {
        return false;
    }
};


// partial specialization: hash bucket datum with at least one key-value pair stored
template<typename KVTypeHead, typename ... KVTypesTail>
class HashBucket <KVTypeHead, KVTypesTail...> : public HashBucket<KVTypesTail...>
{
public:
    template<typename KVType>
    using add_entry_type = HashBucket<KVType, KVTypeHead, KVTypesTail...>;  //!< type of the hash bucket with a new entry added

    static size_t const size = HashBucket<KVTypesTail...>::size + 1;    //!< number of values stored in the hash bucket


    //! Checks if the bucket contains a pair with the specified key. Returns 'true' if provided key has been located.
    //! Returns 'false' otherwise
    template<typename KVType>
    static constexpr bool hasKey()
    {
        return StringComparator<typename KVType::key_string_encapsulator, typename KVTypeHead::key_string_encapsulator>::is_equal() ?
            true :
            HashBucket<KVTypesTail...>:: template hasKey<KVType>();
    }


    //! Attempts to find a value with the given key in the bucket
    template<typename KVType, typename = std::enable_if<StringComparator<typename KVType::key_string_encapsulator, typename KVTypeHead::key_string_encapsulator>::is_equal()>::type>
    typename KVType::value_type const& value() const
    {
        return m_kvtype.value();
    }

    //! Attempts to find a value with the given key in the bucket
    template<typename KVType, typename  = void, typename = typename std::enable_if<!StringComparator<typename KVType::key_string_encapsulator, typename KVTypeHead::key_string_encapsulator>::is_equal()>::type>
    typename KVType::value_type const& value() const
    {
        return HashBucket<KVTypesTail...>::value<KVType>();
    }

    //! Attempts to find a value with the given key in the bucket
    template<typename KVType, typename = std::enable_if<StringComparator<typename KVType::key_string_encapsulator, typename KVTypeHead::key_string_encapsulator>::is_equal()>::type>
    typename KVType::value_type& value()
    {
        return m_kvtype.value();
    }


    //! Attempts to find a value with the given key in the bucket
    template<typename KVType, typename = void, typename = typename std::enable_if<!StringComparator<typename KVType::key_string_encapsulator, typename KVTypeHead::key_string_encapsulator>::is_equal()>::type>
    typename KVType::value_type& value()
    {
        return HashBucket<KVTypesTail...>::value<KVType>();
    }


private:
    KVTypeHead m_kvtype;
};



// implements hash bucket with associated hash value
template<typename H, H hash, typename HashBucketType>
class HashValueHashBucketPair
{
public:
    using hash_value_type = H;  //! type alias of the hash value type
    using hash_bucket_type = HashBucketType;    //! type alias for the hash bucket object encapsulated in the hash value - hash bucket pair


    //! Helper type alias template allowing to add entries to the wrapped hash bucket and to model the corresponding
    //! underlying hash value — hash bucket type. Here we assume that the key is already verified to have the corresponding hash value.
    template<typename KVType>
    using add_entry_type = typename HashValueHashBucketPair<H, hash, typename HashBucketType::template add_entry_type<KVType>>;

    static H const hash_value = hash;

    HashBucketType& bucket()
    {
        return m_hash_bucket;
    }

private:
    HashBucketType m_hash_bucket;
};



// helper template predicate. Evaluates to structure containing type result,
// which evaluated to T1 if Pred = True, and to T2 if Pred = False
template<bool Pred, typename T1, typename T2> struct IfPred;
template<typename T1, typename T2> struct IfPred<true, T1, T2> { using result = T1; };
template<typename T1, typename T2> struct IfPred<false, T1, T2> { using result = T2; };



// auxiliary type needed to represent sets of hash value — hash bucket pairs.
// The hash types and value types should be the same for all pairs in the series
template<template<typename H> class Hash, typename... HashValueHashBucketPairs> struct HashBucketSeries;


// partial specialization: hash value — hash bucket series object with only one element
template<template<typename H> class Hash, typename HeadHashValueHashBucketPair>
struct HashBucketSeries<Hash, HeadHashValueHashBucketPair>
{
public:
    using hash_value_type = typename HeadHashValueHashBucketPair::hash_value_type;  //!< alias for hash value type
    using hash_type = Hash<hash_value_type>;    //!< alias of the hash implementation object type


    //! Inserts new type into the type parameter pack of the object and returns the corresponding combined type
    template<typename NewHeadHashValueHashBucketPair>
    using add_hash_value_hash_bucket_head_type = HashBucketSeries<Hash, NewHeadHashValueHashBucketPair, HeadHashValueHashBucketPair>;


    //! Retrieves hash bucket type corresponding to provided hash value.
    //! If the hash value is not found retrieves the empty hash bucket type.
    template<hash_value_type hash_value>
    using retrieve_hash_bucket_type = typename IfPred<hash_value == HeadHashValueHashBucketPair::hash_value,
        typename HeadHashValueHashBucketPair::hash_bucket_type, HashBucket<>>::result;


    //! Inserts new key — value pair into the hash value — hash bucket series object and returns the corresponding type
    template<typename KVType>
    using add_entry_type =
        typename IfPred<hash_type::template hash<KVType>() == HeadHashValueHashBucketPair::hash_value,
        HashBucketSeries<Hash, typename HeadHashValueHashBucketPair:: template add_entry_type<KVType> >,
        HashBucketSeries<Hash, HeadHashValueHashBucketPair,
        HashValueHashBucketPair<hash_value_type, hash_type::template hash<KVType>(),
        HashBucket<KVType>>> >::result;

    static size_t const num_buckets = 1U;    //!< Number of hash value — hash bucket elements stored in the series object

    template<hash_value_type hash_value>
    retrieve_hash_bucket_type<hash_value>& bucket_from_hash()
    {
        return m_hash_value_hash_bucket_pair.bucket();
    }

private:
    HeadHashValueHashBucketPair m_hash_value_hash_bucket_pair;
};


// partial specialization: hash value — hash bucket series object with at least one element stored
template<template<typename H> class Hash, typename HeadHashValueHashBucketPair, typename ... HashValueHashBucketPairs>
struct HashBucketSeries<Hash, HeadHashValueHashBucketPair, HashValueHashBucketPairs...> :
    public HashBucketSeries<Hash, HashValueHashBucketPairs...>
{
public:
    using hash_value_type = typename HeadHashValueHashBucketPair::hash_value_type;  //!< alias for hash value type
    using hash_type = Hash<hash_value_type>;    //!< alias of the hash implementation object type


    //! Inserts new type into the type parameter pack of the object and returns the corresponding combined type
    template<typename NewHeadHashValueHashBucketPair>
    using add_hash_value_hash_bucket_head_type = HashBucketSeries<Hash, NewHeadHashValueHashBucketPair, HeadHashValueHashBucketPair, HashValueHashBucketPairs...>;


    //! Retrieves hash bucket type corresponding to provided hash value.
    //! If the hash value is not found retrieves empty hash bucket type.
    template<hash_value_type hash_value>
    using retrieve_hash_bucket_type = typename IfPred<hash_value == HeadHashValueHashBucketPair::hash_value,
        typename HeadHashValueHashBucketPair::hash_bucket_type,
        typename HashBucketSeries<Hash, HashValueHashBucketPairs...>:: template retrieve_hash_bucket_type<hash_value> >::result;


    //! Inserts new key — value pair into the hash value — hash bucket series object and returns the corresponding type
    template<typename KVType>
    using add_entry_type =
        typename IfPred<hash_type:: template hash<KVType>() == HeadHashValueHashBucketPair::hash_value,
        HashBucketSeries<Hash, typename HeadHashValueHashBucketPair:: template add_entry_type<KVType>, HashValueHashBucketPairs...>,
        typename HashBucketSeries<Hash, HashValueHashBucketPairs...>:: template add_entry_type<KVType>:: template add_hash_value_hash_bucket_head_type<HeadHashValueHashBucketPair> >::result;


    static size_t const num_buckets = 1 + HashBucketSeries<Hash, HashValueHashBucketPairs...>::num_buckets;  //!< Number of hash value — hash bucket elements stored in the series object


    template<hash_value_type hash_value, typename = typename std::enable_if<hash_value == HeadHashValueHashBucketPair::hash_value>::type>
    typename HeadHashValueHashBucketPair::hash_bucket_type& bucket_from_hash()
    {
        return m_hash_value_hash_bucket_pair.bucket();
    }

    template<hash_value_type hash_value, typename = void, typename = typename std::enable_if<hash_value != HeadHashValueHashBucketPair::hash_value>::type>
    retrieve_hash_bucket_type<hash_value>& bucket_from_hash()
    {
        return HashBucketSeries<Hash, HashValueHashBucketPairs...>::bucket_from_hash<hash_value>();
    }


private:
    HeadHashValueHashBucketPair m_hash_value_hash_bucket_pair;
};



// helper template object that encapsulates hash value — hash bucket series object in a more user friendly manner
template<typename SeriesObjectType>
class StaticHashTableImpl
{
public:
    using hash_value_type = typename SeriesObjectType::hash_value_type;    //! hash value type alias
    using hash_type = typename SeriesObjectType::hash_type;    //!< alias of the hash implementation object type


    //! Checks if provided key is contained in the hash table
    template<typename KVType>
    static constexpr bool hasKey()
    {
        return SeriesObjectType:: template retrieve_hash_bucket_type<hash_type:: template hash<KVType>()>::hasKey<KVType>();
    }

    //! Retrieves value given the search key from the hash table
    template<typename KVType, typename = typename std::enable_if<hasKey<KVType>()>::type>
    typename KVType::value_type const& getValue() const
    {
        return m_hash_value_hash_bucket_series.bucket_from_hash<SeriesObjectType::hash_type:: template hash<KVType>()>().value<KVType>();
    }

    //! Retrieves value given the search key from the hash table
    template<typename KVType, typename = typename std::enable_if<hasKey<KVType>()>::type>
    typename KVType::value_type& getValue()
    {
        return m_hash_value_hash_bucket_series.bucket_from_hash<SeriesObjectType::hash_type:: template hash<KVType>()>().value<KVType>();
    }


    // Special SFINAE-based overloaded templates needed to raise compile time error when requested key is not found in static hash table
    // Adding this static assertion into the normally used template functions does not provide clean solution since in this case compiler
    // in addition to the static assertion fail error will raise other errors due to subsequent template deductions
    // Note: below are two template functions to be invoked in constant and non-constant contexts respectively

    template<typename KVType, typename = void, typename = typename std::enable_if<!hasKey<KVType>()>::type>
    typename KVType::value_type const& getValue() const
    {
        static_assert(false, "requested key is not found in static hash table");
    }

    template<typename KVType, typename = void, typename = typename std::enable_if<!hasKey<KVType>()>::type>
    typename KVType::value_type& getValue()
    {
        static_assert(false, "requested key is not found in static hash table");
    }


private:
    SeriesObjectType m_hash_value_hash_bucket_series;
};



// partial specialization: static hash table with at least one element contained
template<typename H, template<typename H> class Hash, typename KVTypeHead>
class StaticHashTable<H, Hash, KVTypeHead>
{
protected:
    using series_type = HashBucketSeries<Hash, HashValueHashBucketPair<H, Hash<H>:: template hash<KVTypeHead>(), HashBucket<KVTypeHead>>>;

public:
    //!< Adds entry to the static hash table type
    template<typename KVType>
    using add_entry = StaticHashTable<H, Hash, KVType, KVTypeHead>;


    //! Checks if provided key is contained in the hash table
    template<typename KVType>
    static bool hasKey() { return false; }

    //! Retrieves value given the search key from the hash table
    template<typename KVType>
    typename KVType::value_type const& getValue() const
    {
        return m_impl.getValue<KVType>();
    }

    //! Retrieves value given the search key from the hash table
    template<typename KVType>
    typename KVType::value_type& getValue()
    {
        return m_impl.getValue<KVType>();
    }


private:
    StaticHashTableImpl<series_type> m_impl;
};


// partial specialization: static hash table with at least one entry
template<typename H, template<typename H> class Hash, typename KVTypeHead, typename ... KVTypesTail>
class StaticHashTable<H, Hash, KVTypeHead, KVTypesTail...>
{
protected:
    using series_type = typename StaticHashTable<H, Hash, KVTypesTail...>::series_type:: template add_entry_type<KVTypeHead>;

public:
    //!< Adds entry to the static hash table type
    template<typename KVType>
    using add_entry = StaticHashTable<H, Hash, KVType, KVTypeHead, KVTypesTail...>;


    //! Checks if provided key is contained in the hash table
    template<typename KVType>
    static bool hasKey()
    {
        return StaticHashTableImpl<series_type>:: template hasKey<KVType>();
    }

    //! Retrieves value given the search key from the hash table
    template<typename KVType>
    typename KVType::value_type const& getValue() const
    {
        return m_impl.getValue<KVType>();
    }

    //! Retrieves value given the search key from the hash table
    template<typename KVType>
    typename KVType::value_type& getValue()
    {
        return m_impl.getValue<KVType>();
    }

private:
    StaticHashTableImpl<series_type> m_impl;
};



//! Implements 32- and 64- bit FNV1a hashing algorithm for strings
//! For details see http://www.isthe.com/chongo/tech/comp/fnv/index.html#FNV-1
template<typename H> class FNV1aHash;

//! Specialization of FNV1a hashing algorithm for 32-bit hash values
template<> class FNV1aHash<uint32_t>
{
private:
    static constexpr uint64_t FNV_prime = (1U << 24U) + (1U << 8U) + 0x93;
    static constexpr uint32_t offset_bias = 2166136261U;

    //! Helper: allows to retrieve length of a C-string at compile time
    static constexpr size_t c_str_len(char const* c_str)
    {
        return c_str[0] ? 1 + c_str_len(c_str + 1) : 0;
    }


    //! Helper: retrieves data octet from the key
    static uint32_t constexpr get_octet(char const* key, size_t key_len, size_t octet)
    {
        return ((octet * 4 >= key_len ? 0 : key[4 * octet]) << 24) +
            ((octet * 4 + 1 >= key_len ? 0 : key[4 * octet + 1]) << 16) +
            ((octet * 4 + 2 >= key_len ? 0 : key[4 * octet + 2]) << 8) +
            (octet * 4 + 3 >= key_len ? 0 : key[4 * octet + 3]);
    }


    //! Helper: computes hash for all data octets until no octets left
    static uint32_t constexpr gen_hash(char const* key, size_t key_len, size_t octet, uint32_t computed_hash, size_t octets_left)
    {
        return octets_left ? gen_hash(key, key_len, octet + 1, static_cast<uint32_t>((computed_hash ^ get_octet(key, key_len, octet)) * FNV_prime), octets_left - 1) : computed_hash;
    }


    //! Helper: returns number of data octets for the given key
    static size_t constexpr num_octets(char const* key)
    {
        return c_str_len(key) / 4 + (c_str_len(key) % 4 != 0);
    }


public:
    //! Generates hash value given the key
    template<typename KVType>
    static constexpr uint32_t hash()
    {
        return gen_hash(KVType::key, c_str_len(KVType::key), 0, offset_bias, num_octets(KVType::key));
    }
};



//! Specialization of FNV1a hashing algorithm for 64-bit hash values
template<> class FNV1aHash<uint64_t>
{
private:
    static constexpr uint64_t FNV_prime = (static_cast<uint64_t>(1U) << 40U) + (1U << 8U) + 0xb3;
    static constexpr uint64_t offset_bias = 14695981039346656037U;

    //! Helper: allows to retrieve length of a C-string at compile time
    static constexpr size_t c_str_len(char const* c_str)
    {
        return c_str[0] ? 1 + c_str_len(c_str + 1) : 0;
    }


    //! Helper: retrieves data octet from the key
    static uint64_t constexpr get_octet(char const* key, size_t key_len, size_t octet)
    {
        return ((octet * 4 >= key_len ? 0 : key[4 * octet]) << 24) +
            ((octet * 4 + 1 >= key_len ? 0 : key[4 * octet + 1]) << 16) +
            ((octet * 4 + 2 >= key_len ? 0 : key[4 * octet + 2]) << 8) +
            (octet * 4 + 3 >= key_len ? 0 : key[4 * octet + 3]);
    }


    //! Helper: computes hash for all data octets until no octets left
    static uint64_t constexpr gen_hash(char const* key, size_t key_len, size_t octet, uint64_t computed_hash, size_t octets_left)
    {
        return octets_left ? gen_hash(key, key_len, octet + 1, static_cast<uint64_t>((computed_hash ^ get_octet(key, key_len, octet)) * FNV_prime), octets_left - 1) : computed_hash;
    }


    //! Helper: returns number of data octets for the given key
    static size_t constexpr num_octets(char const* key)
    {
        return c_str_len(key) / 4 + (c_str_len(key) % 4 != 0);
    }


public:
    //! Generates hash value given the key
    template<typename KVType>
    static constexpr uint64_t hash()
    {
        return gen_hash(KVType::key, c_str_len(KVType::key), 0, offset_bias, num_octets(KVType::key));
    }
};




#define LEXGINE_SHT_KVPAIR(key, value_type)\
struct __##key##_##value##_static_hash_tables_string_encapsulator{ static constexpr char const* str() { return #key; } };\
using key = typename KeyValuePairCreator<value_type, __##key##_##value##_static_hash_tables_string_encapsulator>::result;

#define LEXGINE_SHT(table_name, ...)\
using table_name = StaticHashTable<uint32_t, FNV1aHash, __VA_ARGS__>;

#define LEXGINE_SHT64(table_name, ...)\
using table_name = StaticHashTable<uint64_t, FNV1aHash, __VA_ARGS__>;

}}}

#endif