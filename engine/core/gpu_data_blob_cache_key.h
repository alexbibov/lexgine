#ifndef LEXGINE_CORE_GPU_DATA_BLOB_CACHE_KEY_H
#define LEXGINE_CORE_GPU_DATA_BLOB_CACHE_KEY_H

#include <array>
#include "engine/core/misc/hash_value.h"
#include "engine/core/misc/uuid.h"

namespace lexgine::core {

class GpuDataBlobCacheKey
{
public:
    struct CommonManifest
    {
        uint32_t magic;
        uint32_t custom_segment_size;
        misc::UUID gpu_driver_uuid;
        std::array<uint8_t, 32> hash;
    };

public:
    static constexpr size_t serialized_size = 256;
    static constexpr size_t custom_segment_size = serialized_size - sizeof(CommonManifest);

public:
    GpuDataBlobCacheKey() = default;

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    GpuDataBlobCacheKey(CommonManifest& common_manifest, T const& data)
        : m_used_bytes{ sizeof(CommonManifest) + sizeof(T) }
        , m_used_words {calculateUsedWords(m_used_bytes)}
    {
        static_assert(sizeof(T) <= custom_segment_size);
        assert(m_used_words >= 1);
        m_data.words[m_used_words - 1] = 0;
        common_manifest.custom_segment_size = sizeof(T);
        memcpy(m_data.bytes, &common_manifest, sizeof(CommonManifest));
        memcpy(m_data.bytes + sizeof(CommonManifest), &data, sizeof(T));
        if constexpr (constexpr size_t rem = custom_segment_size - sizeof(T))
        {
            memset(m_data.bytes + sizeof(CommonManifest) + sizeof(T), 0, rem);
        }
    }

    std::string toString() const;
    size_t hash() const;

    void serialize(void* p_serialization_blob) const;
    void deserialize(void const* p_serialization_blob);

    bool operator<(GpuDataBlobCacheKey const& other) const;
    bool operator==(GpuDataBlobCacheKey const& other) const;

    static CommonManifest createManifest(
        std::array<char, 4> const& magic_bytes,
        misc::UUID const& gpu_driver_uuid,
        misc::HashValue const& hash_value
    );

private:
    static constexpr size_t key_data_size = serialized_size;

private:
    static size_t calculateUsedWords(size_t used_bytes);

private:
    //                 *** serialized data ***
    union MyUnion
    {
        uint64_t words[key_data_size / sizeof(uint64_t)];
        uint8_t bytes[key_data_size];
    } m_data;
    
    //                 ***********************
    size_t m_used_bytes { 0 };
    size_t m_used_words { 0 };
};

struct GpuDataBlobCacheKeyHasher final
{
    size_t operator()(GpuDataBlobCacheKey const& value) const
    {
        return value.hash();
    }
};

}

#endif
