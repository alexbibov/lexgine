#ifndef LEXGINE_CONVERSION_TEXTURE_CONVERTER_H
#define LEXGINE_CONVERSION_TEXTURE_CONTERTER_H

#include <unordered_map>
#include <fstream>
#include <span>
#include <future>
#include <mutex>

#include <engine/core/engine_api.h>
#include <engine/core/globals.h>
#include <engine/core/global_constants.h>
#include <engine/core/streamed_cache.h>
#include <engine/core/entity.h>
#include <engine/core/misc/uuid.h>
#include <engine/core/dx/d3d12/resource.h>
#include <engine/core/dx/d3d12/resource_data_uploader.h>
#include <engine/core/dx/d3d12/upload_buffer_allocator.h>
#include <engine/conversion/class_names.h>
#include <engine/scenegraph/image.h>

#include "lexgine_conversion_fwd.h"


namespace lexgine::conversion
{

struct TextureConversionTaskKey final
{
    char name[4096];
    static constexpr size_t serialized_size = sizeof(name);

    TextureConversionTaskKey() = default;
    TextureConversionTaskKey(std::string const& name);

    std::string toString() const;

    void serialize(void* p_serialization_blob) const;
    void deserialize(void const* p_serialization_blob);

    bool operator<(TextureConversionTaskKey const& other) const;
    bool operator==(TextureConversionTaskKey const& other) const;
};

enum class TextureConversionStatus 
{
    not_started,
    in_progress,
    completed,
    error
};

class TextureUploadWork;
class TextureConversionTask final
{
public:
    TextureConversionTask(TextureConverter& texture_converter, scenegraph::Image& source_image, bool skip_source_image_load);
    void operator()(void);
    TextureUploadWork* getUploadWork() const { return m_texture_upload_work.get(); }
    TextureConversionStatus getStatus() const
    {
        return static_cast<TextureConversionStatus>(m_status.load(std::memory_order_acquire));
    }

private:
    TextureConverter& m_texture_converter;
    scenegraph::Image& m_source_image;
    bool m_skip_source_image_load;
    std::unique_ptr<TextureUploadWork> m_texture_upload_work;
    std::atomic_int m_status;
};

class TextureUploadWork final
{
public:
    TextureUploadWork(TextureConverter& texture_converter,
        TextureConversionTaskKey const& conversion_key,
        core::misc::UUID texture_uuid,
        core::SharedDataChunk const& converted_texture_data,
        conversion::ImageLoader::Description& texture_description,
        core::dx::d3d12::ResourceDataUploader::TextureSourceDescriptor const& source_descriptor);

    bool schedule();
    void evict() { m_converted_texture_data = nullptr; }
    void clear() { m_texture = nullptr; }
    bool isCompleted() const;
    core::misc::UUID uuid() const { return m_texture_uuid; }

    core::dx::d3d12::Resource resource() const { return m_texture; }

private:
    TextureConverter& m_texture_converter;
    TextureConversionTaskKey m_conversion_key;
    core::misc::UUID m_texture_uuid;
    core::SharedDataChunk m_converted_texture_data;
    core::dx::d3d12::ResourceDataUploader::TextureSourceDescriptor m_src_desc;
    core::dx::d3d12::Resource m_texture;
    uint64_t m_controlling_signal;
};


}

namespace std
{
    template<>
    struct hash<lexgine::conversion::TextureConversionTaskKey>
    {
        size_t operator()(lexgine::conversion::TextureConversionTaskKey const& key) const noexcept
        {
            lexgine::core::misc::HashedString hashed_string{ key.toString()};
            return static_cast<size_t>(hashed_string.hash());
        }
    };
}

namespace lexgine::conversion
{

class TextureConverter : public core::NamedEntity<class_names::TextureConverter>
{
    friend class TextureConversionTask;
    friend class TextureUploadWork;
private:
    class sha256_provider final
    {
    public:
        static constexpr unsigned c_hash_length = 32;

        sha256_provider(TextureConverter* context);
        ~sha256_provider();
        std::array<uint8_t, c_hash_length> hash(std::span<uint8_t const> data) const;

    private:
        TextureConverter* m_context;
        void* m_algorithm_handle;
        void* m_hash_object_handle;
        std::vector<uint8_t> m_hash_object_buffer;
    };

    struct CachedTextureData
    {
        core::SharedDataChunk data;
        conversion::ImageLoader::Description description;
        core::dx::d3d12::ResourceDataUploader::TextureSourceDescriptor source_descriptor;
    };

    using TextureCache = core::StreamedCache<TextureConversionTaskKey, core::global_constants::combined_cache_cluster_size>;
    using TextureTasksCache = std::unordered_map<TextureConversionTaskKey, TextureConversionTask>;

public:
    TextureConverter(core::Globals& globals);
    ~TextureConverter();

    TextureConversionTask const* addTextureConversionTask(scenegraph::Image& source_image, bool skip_source_image_load);
    core::dx::d3d12::ResourceDataUploader& getDataUploader() { return m_data_uploader; }

    void convertTextures(uint32_t thread_count = static_cast<uint32_t>(-1));
    void uploadTextures();

    bool isTextureConversionCompleted() const;
    bool isTextureUploadCompleted() const;

    void waitForTextureConversionCompletion();
    void waitForTextureUploadCompletion();

private:
    CachedTextureData readTextureFromCache(TextureConversionTaskKey const& key, core::misc::UUID& uuid) const;
    static [[nodiscard]] TextureConversionTaskKey createConversionTaskKey(scenegraph::Image& source_image);

private:
    core::Globals& m_globals;
    
    core::dx::d3d12::DedicatedUploadDataStreamAllocator m_upload_stream_allocator;
    core::dx::d3d12::ResourceDataUploader m_data_uploader;

    std::unique_ptr<sha256_provider> m_sha256_provider;
    TextureTasksCache m_texture_conversion_tasks;
    std::vector<std::future<void>> m_texture_conversion_futures;

    std::mutex m_texture_cache_mutex;
    std::fstream m_cache_stream;
    std::unique_ptr<TextureCache> m_compressed_textures_cache;
};

}

#endif
