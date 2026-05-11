#ifndef LEXGINE_CORE_GPU_DATA_BLOB_CACHE_KEY_H
#define LEXGINE_CORE_GPU_DATA_BLOB_CACHE_KEY_H

#include <concepts>
#include <array>
#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/misc/uuid.h"

namespace lexgine::core {

class GpuDataBlobCacheKey
{
public:
    struct CommonManifest
    {
        uint32_t magic;
        uint32_t reserved;
        misc::UUID gpu_driver_uuid;
        std::array<uint8_t, 32> hash;
    };
public:
    static constexpr size_t serialized_size = 256;
    static constexpr size_t custom_segment_size = serialized_size - sizeof(CommonManifest);

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    explicit GpuDataBlobCacheKey(CommonManifest const& common_manifest, T const& data)
        :  m_used_bytes{ sizeof(CommonManifest) + sizeof(T) }
    {
        static_assert(sizeof(T) <= sizeof(custom_segment_size));
        m_used_words = ((m_used_bytes + 7) & (~7)) / 8;
        assert(m_used_words >= 1);
        m_data.words[m_used_words - 1] = 0;
        memcpy(&m_data, &common_manifest, sizeof(CommonManifest));
        memcpy(&m_data + sizeof(CommonManifest), &data, m_used_bytes);
    }

    std::string toString() const;

    void serialize(void* p_serialization_blob) const;
    void deserialize(void const* p_serialization_blob);

    bool operator<(GpuDataBlobCacheKey const& other) const;
    bool operator==(GpuDataBlobCacheKey const& other) const;

private:
    static constexpr size_t key_data_size = serialized_size;

private:
    //                 *** serialized data ***
    union MyUnion
    {
        uint64_t words[key_data_size / sizeof(uint64_t)];
        uint8_t bytes[key_data_size];
    } m_data;
    
    //                 ***********************
    size_t m_used_bytes;
    size_t m_used_words;
};

}

#endif
