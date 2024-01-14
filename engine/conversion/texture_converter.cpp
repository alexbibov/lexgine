#include "DirectXTex.h"

#include <bcrypt.h>

#include <algorithm>
#include <iterator>
#include <filesystem>

#include <engine/core/global_settings.h>
#include <engine/core/logging_streams.h>
#include <engine/core/dx/d3d12/device.h>
#include <engine/core/dx/d3d12/dx_resource_factory.h>
#include <engine/core/dx/d3d12/d3d12_tools.h>
#include <engine/core/exception.h>
#include <engine/core/misc/strict_weak_ordering.h>
#include <engine/core/misc/misc.h>
#include "texture_converter.h"

#define STATUS_SUCCESS 0

using namespace lexgine::core;

namespace lexgine::conversion
{

std::string const TextureConverter::c_upload_section_name = "texture_upload_section";

namespace
{

template<typename T>
union value_packer
{
    T value;
    uint8_t bytes[sizeof(T)];
};

using uint64_t_packer = value_packer<uint64_t>;

core::GlobalSettings const* getGlobalSettings(core::Globals const& globals)
{
    return globals.get<core::GlobalSettings>();
}

DXGI_FORMAT getImageFormat(core::Globals const& globals, conversion::ImageLoader::Description const& image_desc)
{
    dx::d3d12::DxgiFormatFetcher const& format_fetcher = globals.get<dx::d3d12::DxResourceFactory>()->dxgiFormatFetcher();
    auto const color_space = image_desc.color_space;

    bool const is_fp = color_space == conversion::ImageColorSpace::hdr;
    size_t const element_count = image_desc.element_count;
    size_t const element_size = image_desc.element_size;
    return format_fetcher.fetch(is_fp, !image_desc.is_unsigned, false, element_count, element_size);
}

size_t getCompressedImageBlockSize(lexgine::conversion::ImageCompressedDataFormat compression_format)
{
    switch (compression_format)
    {
    case lexgine::conversion::ImageCompressedDataFormat::bc1_unorm:
    case lexgine::conversion::ImageCompressedDataFormat::bc1_unorm_srgb:
    case lexgine::conversion::ImageCompressedDataFormat::bc4_unorm:
    case lexgine::conversion::ImageCompressedDataFormat::bc4_snorm:
        return 8;

    case lexgine::conversion::ImageCompressedDataFormat::bc2_unorm:
    case lexgine::conversion::ImageCompressedDataFormat::bc2_unorm_srgb:
    case lexgine::conversion::ImageCompressedDataFormat::bc3_unorm:
    case lexgine::conversion::ImageCompressedDataFormat::bc3_unorm_srgb:
    case lexgine::conversion::ImageCompressedDataFormat::bc5_unorm:
    case lexgine::conversion::ImageCompressedDataFormat::bc5_snorm:
    case lexgine::conversion::ImageCompressedDataFormat::bc6h_uf16:
    case lexgine::conversion::ImageCompressedDataFormat::bc6h_sf16:
    case lexgine::conversion::ImageCompressedDataFormat::bc7_unorm:
    case lexgine::conversion::ImageCompressedDataFormat::bc7_unorm_srgb:
        return 16;

    case lexgine::conversion::ImageCompressedDataFormat::unknown:
    case lexgine::conversion::ImageCompressedDataFormat::no_compression:
        return 1;

    default:
        return 1;
    }
}

void updateImageDescForcompressionFormat(conversion::ImageCompressedDataFormat compressed_format, conversion::ImageLoader::Description& desc)
{
    desc.element_size = getCompressedImageBlockSize(compressed_format);
    desc.compression_format = compressed_format;

    switch (compressed_format)
    {
    case lexgine::conversion::ImageCompressedDataFormat::bc1_unorm:
    case lexgine::conversion::ImageCompressedDataFormat::bc2_unorm:
    case lexgine::conversion::ImageCompressedDataFormat::bc3_unorm:
    case lexgine::conversion::ImageCompressedDataFormat::bc7_unorm:
        desc.color_space = conversion::ImageColorSpace::rgb;
        desc.element_count = 4;
        desc.is_unsigned = true;
        break;

    case lexgine::conversion::ImageCompressedDataFormat::bc1_unorm_srgb:
    case lexgine::conversion::ImageCompressedDataFormat::bc2_unorm_srgb:
    case lexgine::conversion::ImageCompressedDataFormat::bc3_unorm_srgb:
    case lexgine::conversion::ImageCompressedDataFormat::bc7_unorm_srgb:
        desc.color_space = conversion::ImageColorSpace::srgb;
        desc.element_count = 4;
        desc.is_unsigned = true;
        break;
     
    case lexgine::conversion::ImageCompressedDataFormat::bc4_unorm:
        desc.color_space = conversion::ImageColorSpace::rgb;
        desc.element_count = 1;
        desc.is_unsigned = true;
        break;

    case lexgine::conversion::ImageCompressedDataFormat::bc4_snorm:
        desc.color_space = conversion::ImageColorSpace::rgb;
        desc.element_count = 1;
        desc.is_unsigned = false;
        break;

    case lexgine::conversion::ImageCompressedDataFormat::bc5_unorm:
        desc.color_space = conversion::ImageColorSpace::rgb;
        desc.element_count = 2;
        desc.is_unsigned = true;
        break;

    case lexgine::conversion::ImageCompressedDataFormat::bc5_snorm:
        desc.color_space = conversion::ImageColorSpace::rgb;
        desc.element_count = 2;
        desc.is_unsigned = false;
        break;

    case lexgine::conversion::ImageCompressedDataFormat::bc6h_uf16:
        desc.color_space = conversion::ImageColorSpace::hdr;
        desc.element_count = 3;
        desc.is_unsigned = true;
        break;

    case lexgine::conversion::ImageCompressedDataFormat::bc6h_sf16:
        desc.color_space = conversion::ImageColorSpace::hdr;
        desc.element_count = 3;
        desc.is_unsigned = false;
        break;
    
    default:
        LEXGINE_ASSUME;
    }
}

dx::d3d12::DedicatedUploadDataStreamAllocator createUploadStreamAllocator(core::Globals& globals)
{
    GlobalSettings const& global_settings = *globals.get<GlobalSettings>();
    dx::d3d12::DxResourceFactory& dx_resource_factory = *globals.get<dx::d3d12::DxResourceFactory>();
    dx::d3d12::Heap& upload_heap = dx_resource_factory.retrieveUploadHeap(*globals.get<dx::d3d12::Device>());
    auto upload_heap_section = dx_resource_factory.allocateSectionInUploadHeap(upload_heap, TextureConverter::c_upload_section_name, global_settings.getTextureUploadPartitionSize());
    if (!upload_heap_section.isValid()) {
        LEXGINE_THROW_ERROR("Unable to create texture upload heap");
    }

    dx::d3d12::UploadHeapPartition const upload_heap_partition = static_cast<dx::d3d12::UploadHeapPartition const>(upload_heap_section);
    return dx::d3d12::DedicatedUploadDataStreamAllocator{ globals, upload_heap_partition.offset, upload_heap_partition.size };
}

void packUint64ToArray(uint64_t value_to_pack, uint8_t* output_buffer, size_t& write_offset)
{
    uint64_t_packer p{};
    p.value = value_to_pack;
    std::copy(p.bytes, p.bytes + sizeof(p), output_buffer + write_offset);
    write_offset += sizeof(p);
}

uint64_t unpackUint64FromArray(uint8_t const* buffer, size_t& read_offset)
{
    uint64_t_packer p{};
    std::copy(buffer + read_offset, buffer + read_offset + sizeof(p), p.bytes);
    read_offset += sizeof(p);
    return p.value;
}

size_t calculateBlobPreambleSizeForImage(conversion::ImageLoader::Description const& image_desc, size_t hash_size)
{
    size_t all_mipmaps_count = std::accumulate(image_desc.layers.begin(), image_desc.layers.end(), 0ULL, [](size_t acc, conversion::ImageLoader::Layer const& layer) { return acc + layer.mipmaps.size(); });
    return all_mipmaps_count * 24 + sizeof(UUID) + 8 + hash_size;
}

}  // anonymous namespace


TextureConverter::sha256_provider::sha256_provider(TextureConverter* context)
    : m_context{ context }
{
    BCRYPT_ALG_HANDLE sha256_algorithm_handle{};
    LEXGINE_THROW_ERROR_IF_FAILED(context, BCryptOpenAlgorithmProvider(&sha256_algorithm_handle, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_HASH_REUSABLE_FLAG), STATUS_SUCCESS);
    m_algorithm_handle = static_cast<void*>(sha256_algorithm_handle);

    union {
        DWORD value;
        unsigned char bytes[sizeof(DWORD)];
    }dword_buf{ 0 };

    ULONG bytes_written{ 0 };
    LEXGINE_THROW_ERROR_IF_FAILED(context, BCryptGetProperty(sha256_algorithm_handle, BCRYPT_OBJECT_LENGTH, static_cast<PUCHAR>(dword_buf.bytes), sizeof(dword_buf), &bytes_written, NULL), STATUS_SUCCESS);
    m_hash_object_buffer.resize(dword_buf.value);


    BCRYPT_HASH_HANDLE hash_handle{};
    LEXGINE_THROW_ERROR_IF_FAILED(context, BCryptCreateHash(sha256_algorithm_handle, &hash_handle, static_cast<unsigned char*>(m_hash_object_buffer.data()), static_cast<ULONG>(m_hash_object_buffer.size()), NULL, 0, BCRYPT_HASH_REUSABLE_FLAG), STATUS_SUCCESS);
    m_hash_object_handle = static_cast<void*>(hash_handle);

    dword_buf.value = 0;
    LEXGINE_THROW_ERROR_IF_FAILED(context, BCryptGetProperty(sha256_algorithm_handle, BCRYPT_HASH_LENGTH, static_cast<PUCHAR>(dword_buf.bytes), sizeof(dword_buf), &bytes_written, NULL), STATUS_SUCCESS);
    assert(dword_buf.value == sha256_provider::c_hash_length);
}

TextureConverter::sha256_provider::~sha256_provider()
{
    LEXGINE_LOG_ERROR_IF_FAILED(m_context, BCryptDestroyHash(static_cast<BCRYPT_HASH_HANDLE>(m_hash_object_handle)), STATUS_SUCCESS);
    LEXGINE_LOG_ERROR_IF_FAILED(m_context, BCryptCloseAlgorithmProvider(static_cast<BCRYPT_ALG_HANDLE>(m_algorithm_handle), NULL), STATUS_SUCCESS);
}

std::array<uint8_t, TextureConverter::sha256_provider::c_hash_length> TextureConverter::sha256_provider::hash(std::span<uint8_t const> data) const
{
    std::array<uint8_t, c_hash_length> rv{};
    LEXGINE_THROW_ERROR_IF_FAILED(m_context, BCryptHashData(static_cast<BCRYPT_HASH_HANDLE>(m_hash_object_handle), static_cast<PUCHAR>(const_cast<uint8_t*>(data.data())), static_cast<ULONG>(data.size_bytes()), NULL), STATUS_SUCCESS);
    LEXGINE_THROW_ERROR_IF_FAILED(m_context, BCryptFinishHash(static_cast<BCRYPT_HASH_HANDLE>(m_hash_object_handle), static_cast<PUCHAR>(rv.data()), static_cast<ULONG>(rv.size()), NULL), STATUS_SUCCESS);

    return rv;
}


TextureConverter::TextureConversionTaskKey::TextureConversionTaskKey(std::string const& name)
    : name{}
{
    std::copy(name.begin(), name.end(), this->name);
}

std::string TextureConverter::TextureConversionTaskKey::toString() const
{
    return std::string{ name };
}

void TextureConverter::TextureConversionTaskKey::serialize(void* p_serialization_blob) const
{
    uint8_t* p_serialization_buffer = reinterpret_cast<uint8_t*>(p_serialization_blob);
    std::copy(name, name + sizeof(name) / sizeof(name[0]), p_serialization_buffer);
}

void TextureConverter::TextureConversionTaskKey::deserialize(void const* p_serialization_blob)
{
    uint8_t const* p_serialization_buffer = reinterpret_cast<uint8_t const*>(p_serialization_blob);
    std::copy(p_serialization_buffer, p_serialization_buffer + sizeof(name), name);
}

bool TextureConverter::TextureConversionTaskKey::operator<(TextureConversionTaskKey const& other) const
{
    SWO_END(std::string{ name }, < , std::string{ other.name });
}

bool TextureConverter::TextureConversionTaskKey::operator==(TextureConversionTaskKey const& other) const
{
    return std::equal(name, name + sizeof(name) / sizeof(name[0]), other.name);
}


TextureConverter::TextureConversionTask::TextureConversionTask(TextureConverter& texture_converter, scenegraph::Image& source_image, bool skip_source_image_load)
    : m_texture_converter{ texture_converter }
    , m_skip_source_image_load{ skip_source_image_load }
    , m_source_image{ source_image }
{
    std::fill(m_key.name, m_key.name + sizeof(m_key.name), 0);
    std::string const& image_uri = source_image.uri();
    assert(image_uri.size() <= sizeof(m_key.name));
    std::copy(image_uri.begin(), image_uri.end(), m_key.name);
}


TextureConverter::TextureConversionTask::result_type TextureConverter::TextureConversionTask::operator()(void) const
{
    std::scoped_lock<std::mutex> lock{ m_texture_converter.m_texture_cache_mutex };

    bool should_convert = !m_texture_converter.m_compressed_textures_cache->doesEntryExist(m_key);
    std::array<uint8_t, 32U> sha256{};    // hash value of the source image (calculated only when should_convert is true)
    if (should_convert)
    {
        if (m_skip_source_image_load || !m_source_image.load())
        {
            return nullptr;
        }

        sha256 = m_texture_converter.m_sha256_provider->hash(std::span<std::uint8_t const>{m_source_image.data(), m_source_image.size()});
    }
    else if(!m_skip_source_image_load)
    {
        if (!m_source_image.load())
        {
            return nullptr;
        }

        misc::DateTime datestamp = m_texture_converter.m_compressed_textures_cache->getEntryTimestamp(m_key);
        if (datestamp < m_source_image.description().timestamp)
        {
            // Calculate hash value of the source image
            sha256 = m_texture_converter.m_sha256_provider->hash(std::span<std::uint8_t const>{m_source_image.data(), m_source_image.size()});
            lexgine::core::SharedDataChunk data_chunk = m_texture_converter.m_compressed_textures_cache->retrieveEntry(m_key);
            void* p_data = data_chunk.data();
            std::array<uint8_t, 32U> cached_sha256{};
            std::copy(static_cast<uint8_t*>(p_data), static_cast<uint8_t*>(p_data) + cached_sha256.size(), cached_sha256.begin());
            should_convert = !std::equal(sha256.begin(), sha256.end(), cached_sha256.begin());
        }
    }

    if (should_convert) {
        auto image_desc = m_source_image.description();

        misc::UUID uuid = misc::UUID::generate();
        core::SharedDataChunk scratch_blob_data{ m_source_image.size() + calculateBlobPreambleSizeForImage(image_desc, sha256_provider::c_hash_length) };

        auto p_device = m_texture_converter.m_globals.get<core::dx::d3d12::Device>();
        auto nativeD3d11Device = p_device->nativeD3d11();

        size_t element_count = image_desc.element_count;
        conversion::ImageCompressedDataFormat current_compression{ image_desc.compression_format };
        conversion::ImageCompressedDataFormat target_compression_format{ current_compression };
        DXGI_FORMAT source_image_format{ DXGI_FORMAT_UNKNOWN };
        std::function<bool(DirectX::Image const&, DirectX::ScratchImage&)> compressor{ [](DirectX::Image const&, DirectX::ScratchImage&) { return false; } };

        if (current_compression == conversion::ImageCompressedDataFormat::no_compression)
        {
            source_image_format = static_cast<DXGI_FORMAT>(getImageFormat(m_texture_converter.m_globals, image_desc));

            DirectX::TEX_COMPRESS_FLAGS compression_flags{ DirectX::TEX_COMPRESS_PARALLEL };
            switch (element_count) {
            case 1:
                target_compression_format = image_desc.is_unsigned
                    ? conversion::ImageCompressedDataFormat::bc4_unorm
                    : conversion::ImageCompressedDataFormat::bc4_snorm;
                break;

            case 2:
                target_compression_format = image_desc.is_unsigned
                    ? conversion::ImageCompressedDataFormat::bc5_unorm
                    : conversion::ImageCompressedDataFormat::bc5_snorm;
                break;

            case 3:
            case 4:
                if (image_desc.color_space != ImageColorSpace::none)
                {
                    switch (image_desc.color_space)
                    {
                    case conversion::ImageColorSpace::rgb:
                        assert(image_desc.is_unsigned);
                        target_compression_format = conversion::ImageCompressedDataFormat::bc7_unorm;
                        compression_flags |= DirectX::TEX_COMPRESS_BC7_USE_3SUBSETS;
                        break;

                    case conversion::ImageColorSpace::srgb:
                        assert(image_desc.is_unsigned);
                        target_compression_format = conversion::ImageCompressedDataFormat::bc7_unorm_srgb;
                        compression_flags |= DirectX::TEX_COMPRESS_BC7_USE_3SUBSETS | DirectX::TEX_COMPRESS_SRGB;
                        break;

                    case conversion::ImageColorSpace::hdr:
                        target_compression_format = image_desc.is_unsigned
                            ? conversion::ImageCompressedDataFormat::bc6h_uf16
                            : conversion::ImageCompressedDataFormat::bc6h_sf16;
                        break;

                    default:
                        LEXGINE_ASSUME;
                    }
                }
                else
                {
                    target_compression_format = conversion::ImageCompressedDataFormat::bc7_unorm;
                }

                break;

            default:
                LEXGINE_ASSUME;
            }

            // Select compression routine (either CPU or GPU)

            if (nativeD3d11Device && target_compression_format >= conversion::ImageCompressedDataFormat::bc6h_uf16
                && target_compression_format < conversion::ImageCompressedDataFormat::unknown) {
                // use GPU compression
                compressor = [this, nativeD3d11Device, target_compression_format, compression_flags](DirectX::Image const& srcImage, DirectX::ScratchImage& dstImage)
                    {
                        LEXGINE_LOG_ERROR_IF_FAILED(m_texture_converter, DirectX::Compress(nativeD3d11Device.Get(), srcImage, static_cast<DXGI_FORMAT>(target_compression_format), compression_flags, DirectX::TEX_THRESHOLD_DEFAULT, dstImage), S_OK);
                        return true;
                    };
            }
            else
            {
                // fall back to CPU compression
                DirectX::ScratchImage compressed_img{};
                compressor = [this, target_compression_format, compression_flags](DirectX::Image const& srcImage, DirectX::ScratchImage& dstImage)
                    {
                        DirectX::TEX_COMPRESS_FLAGS flags = compression_flags;
                        #ifndef _OPENMP
                        if (compression_flags & DirectX::TEX_COMPRESS_PARALLEL)
                        {
                            flags = static_cast<DirectX::TEX_COMPRESS_FLAGS>(static_cast<unsigned long>(compression_flags) ^ DirectX::TEX_COMPRESS_PARALLEL);
                        }
                        #endif

                        LEXGINE_LOG_ERROR_IF_FAILED(m_texture_converter, DirectX::Compress(srcImage, static_cast<DXGI_FORMAT>(target_compression_format), flags, DirectX::TEX_THRESHOLD_DEFAULT, dstImage), S_OK);
                        return true;
                    };
            }
        }
        else
        {
            source_image_format = static_cast<DXGI_FORMAT>(image_desc.compression_format);
        }

        assert(source_image_format != DXGI_FORMAT_UNKNOWN);


        {
            // Store image in the texture cache (while compressing it when required) and prepare texture upload task
            auto& layers = image_desc.layers;

            std::copy(sha256.begin(), sha256.end(), static_cast<uint8_t*>(scratch_blob_data.data()));
            size_t blob_data_write_offset{ sha256_provider::c_hash_length };


            packUint64ToArray(uuid.hiPart(), static_cast<uint8_t*>(scratch_blob_data.data()), blob_data_write_offset);
            packUint64ToArray(uuid.loPart(), static_cast<uint8_t*>(scratch_blob_data.data()), blob_data_write_offset);

            packUint64ToArray(static_cast<uint64_t>(target_compression_format), static_cast<uint8_t*>(scratch_blob_data.data()), blob_data_write_offset);

            dx::d3d12::ResourceDataUploader::TextureSourceDescriptor texture_source_descriptor{};
            texture_source_descriptor.subresources.reserve(image_desc.subresource_count);

            for (size_t layer_id = 0; layer_id < layers.size(); ++layer_id)
            {
                auto& current_layer = layers[layer_id];
                for (size_t mipmap_level_id = 0; mipmap_level_id < current_layer.mipmaps.size(); ++mipmap_level_id)
                {
                    auto& current_mipmap_level = current_layer.mipmaps[mipmap_level_id];

                    glm::uvec3 source_texture_dimensions = current_mipmap_level.dimensions;
                    size_t texture_row_pitch = source_texture_dimensions.x * image_desc.element_count * image_desc.element_size;
                    size_t texture_slice_pitch = texture_row_pitch * source_texture_dimensions.y;

                    uint8_t const* compressed_img_pixels{ nullptr };
                    size_t compressed_img_size{}, compressed_img_row_pitch{}, compressed_img_slice_pitch{};

                    {
                        // Image compression
                        DirectX::Image img{ .width = source_texture_dimensions.x, .height = source_texture_dimensions.y,
                        .format = source_image_format, .rowPitch = texture_row_pitch, .slicePitch = texture_slice_pitch,
                        .pixels = const_cast<uint8_t*>(m_source_image.data() + current_mipmap_level.offset)
                        };

                        DirectX::ScratchImage compressed_img{};


                        if (compressor(img, compressed_img))    // when compression is not needed this is a no-op, which returns 'false')
                        {
                            compressed_img_pixels = compressed_img.GetPixels();
                            compressed_img_size = compressed_img.GetPixelsSize();
                            compressed_img_row_pitch = compressed_img.GetImage(0, 0, 0)->rowPitch;
                            compressed_img_slice_pitch = compressed_img.GetImage(0, 0, 0)->slicePitch;
                        }
                        else
                        {
                            // compressed_img does not contain any data in this branch
                            compressed_img_pixels = img.pixels;
                            compressed_img_size = compressed_img_slice_pitch = img.slicePitch;
                            compressed_img_row_pitch = img.rowPitch;
                        }
                    }
                    packUint64ToArray(compressed_img_size, static_cast<uint8_t*>(scratch_blob_data.data()), blob_data_write_offset);
                    packUint64ToArray(compressed_img_row_pitch, static_cast<uint8_t*>(scratch_blob_data.data()), blob_data_write_offset);
                    packUint64ToArray(compressed_img_slice_pitch, static_cast<uint8_t*>(scratch_blob_data.data()), blob_data_write_offset);

                    uint8_t* subresource_address = static_cast<uint8_t*>(scratch_blob_data.data()) + blob_data_write_offset;
                    std::copy(compressed_img_pixels, compressed_img_pixels + compressed_img_size, subresource_address);
                    blob_data_write_offset += compressed_img_size;

                    texture_source_descriptor.subresources.push_back({ .p_data = subresource_address, .row_pitch = compressed_img_row_pitch, .slice_pitch = compressed_img_slice_pitch });
                }
            }

            // add compressed data to the cache

            SharedDataChunk blob_data{ blob_data_write_offset };
            std::copy(static_cast<uint8_t*>(scratch_blob_data.data()), static_cast<uint8_t*>(scratch_blob_data.data()) + blob_data_write_offset, static_cast<uint8_t*>(blob_data.data()));
            m_texture_converter.m_compressed_textures_cache->addEntry(TextureCache::entry_type{ m_key, blob_data });

            conversion::ImageLoader::Description compressed_texture_description = image_desc;
            compressed_texture_description.compression_format = target_compression_format;
            m_texture_upload_task = std::make_unique<TextureUploadTask>(m_texture_converter, m_key, uuid, scratch_blob_data, compressed_texture_description, texture_source_descriptor);
        }
    }
    else {
        misc::UUID uuid{};
        auto cached_texture_data = m_texture_converter.readTextureFromCache(m_key, uuid);
        m_texture_upload_task = std::make_unique<TextureUploadTask>(m_texture_converter, m_key, uuid, cached_texture_data.data, cached_texture_data.description, cached_texture_data.source_descriptor);
    }

    return m_texture_upload_task.get();
}

size_t TextureConverter::TextureConversionTaskHasher::operator()(TextureConversionTask const& task) const noexcept
{
    core::misc::HashedString hashed_string{ task.key().toString() };
    return static_cast<size_t>(hashed_string.hash());
}

TextureConverter::TextureUploadTask::TextureUploadTask(TextureConverter& texture_converter,
    TextureConversionTaskKey const& conversion_key,
    core::misc::UUID texture_uuid,
    core::SharedDataChunk const& converted_texture_data,
    conversion::ImageLoader::Description& texture_description,
    core::dx::d3d12::ResourceDataUploader::TextureSourceDescriptor const& source_descriptor)
    : m_texture_converter{ texture_converter }
    , m_conversion_key{ conversion_key }
    , m_texture_uuid{ texture_uuid }
    , m_converted_texture_data{ converted_texture_data }
    , m_texture_description{ texture_description }
    , m_src_desc{ source_descriptor }
{
    auto* pDevice = m_texture_converter.m_globals.get<dx::d3d12::Device>();

    glm::uvec3 dimensions = texture_description.layers[0].mipmaps[0].dimensions;
    auto desc = core::dx::d3d12::ResourceDescriptor::CreateTexture2D(static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), static_cast<uint32_t>(dimensions.z),
        static_cast<DXGI_FORMAT>(texture_description.compression_format), static_cast<uint32_t>(texture_description.layers[0].mipmaps.size()),
        core::dx::d3d12::ResourceFlags::base_values::none, core::MultiSamplingFormat{ 1, 0 }, core::dx::d3d12::ResourceAlignment::_default, core::dx::d3d12::TextureLayout::unknown);

    m_texture = std::make_shared<core::dx::d3d12::CommittedResource>(*pDevice, core::dx::d3d12::ResourceState::base_values::pixel_shader, misc::makeEmptyOptional<core::dx::d3d12::ResourceOptimizedClearValue>(), desc, dx::d3d12::AbstractHeapType::_default,
        core::dx::d3d12::HeapCreationFlags::base_values::allow_all);
}


TextureConverter::TextureUploadTask::result_type TextureConverter::TextureUploadTask::operator()(void)
{
    core::dx::d3d12::ResourceDataUploader::DestinationDescriptor::DestinationSegment destination_segment{};
    destination_segment.subresources = core::dx::d3d12::ResourceDataUploader::DestinationDescriptor::SubresourceSegment{ .first_subresource = 0, .num_subresources = static_cast<uint32_t>(m_src_desc.subresources.size()) };
    core::dx::d3d12::ResourceDataUploader::DestinationDescriptor data_upload_destination_descriptor{
        .p_destination_resource = m_texture.get(), 
        .destination_resource_state = core::dx::d3d12::ResourceState::base_values::pixel_shader, 
        .segment = destination_segment
    };

    if (isEvicted())
    {
        auto cached_texture_data = m_texture_converter.readTextureFromCache(m_conversion_key, m_texture_uuid);
        m_converted_texture_data = cached_texture_data.data;
        m_texture_description = cached_texture_data.description;
        m_src_desc = cached_texture_data.source_descriptor;
    }

    m_texture_converter.m_data_uploader.addResourceForUpload(data_upload_destination_descriptor, m_src_desc);

    return m_texture;
}


TextureConverter::TextureConverter(core::Globals& globals)
    : m_globals{ globals }
    , m_sha256_provider{ new sha256_provider{this} }
    , m_upload_stream_allocator{ createUploadStreamAllocator(globals) }
    , m_data_uploader{ globals, m_upload_stream_allocator }
{
    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize{ RO_INIT_MULTITHREADED };
    if (FAILED(initialize))
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "TextureConverter initialization failed");
    }

    {
        auto global_settings = getGlobalSettings(globals);
        std::filesystem::path cache_name = std::filesystem::path{ global_settings->getCacheDirectory() } / (global_settings->getCombinedCacheName() + ".texturedata");
        auto cache_stream_mode = std::ios::in | std::ios::out | std::ios::binary;
        bool cache_exists = std::filesystem::exists(cache_name);
        if (!cache_exists) cache_stream_mode |= std::ios::trunc;
        m_cache_stream.open(cache_name.string(), cache_stream_mode);
        if (!m_cache_stream) {
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Unable to open cache stream for '" + cache_name.string() + "'");
        }

        if (cache_exists) {
            m_compressed_textures_cache.reset(new core::StreamedCache<TextureConversionTaskKey, core::global_constants::combined_cache_cluster_size>{ m_cache_stream });
        }
        
        if (!m_compressed_textures_cache || !(*m_compressed_textures_cache))
        {
            // This branch will be executed in case if the cache either didn't exist or was corrupted
            if(cache_exists && !(*m_compressed_textures_cache))
            {
                // cache is corrupted
                m_cache_stream.close();
                m_cache_stream.open(cache_name.string(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
            }

            m_compressed_textures_cache.reset(new core::StreamedCache<TextureConversionTaskKey, core::global_constants::combined_cache_cluster_size>{ m_cache_stream, global_settings->getMaxCombinedTextureCacheSize(), lexgine::core::StreamedCacheCompressionLevel::level0, true });
        }
    }
}

TextureConverter::~TextureConverter()
{
    m_compressed_textures_cache->finalize();
    m_cache_stream.close();
}

void TextureConverter::addTextureConversionTask(scenegraph::Image& source_image, bool skip_source_image_load)
{

    m_texture_conversion_tasks.emplace(*this, source_image, skip_source_image_load);
}

void TextureConverter::convertTextures(uint32_t thread_count)
{
    waitForTextureConversionCompletion();

    if (thread_count == static_cast<uint32_t>(-1))
        thread_count = std::thread::hardware_concurrency();

    m_all_conversion_tasks_ordered.clear();
    m_all_conversion_tasks_ordered.resize(m_texture_conversion_tasks.size());
    std::transform(m_texture_conversion_tasks.begin(), m_texture_conversion_tasks.end(), m_all_conversion_tasks_ordered.begin(), [](auto& e) {return &e; });

    m_texture_upload_tasks.clear();
    m_texture_upload_tasks.resize(m_all_conversion_tasks_ordered.size());

    struct TextureConversionThreadBucket
    {
        size_t first;
        size_t count;
    };
    std::array<TextureConversionThreadBucket, 128> thread_buckets{};

    size_t bucket_size = m_all_conversion_tasks_ordered.size() / thread_count;
    size_t remainder = m_all_conversion_tasks_ordered.size() % thread_count;

    uint32_t active_threads = bucket_size > 0 ? thread_count : remainder;
    for (size_t i = 0; i < active_threads; ++i)
    {
        thread_buckets[i].first = i * bucket_size + (i < remainder ? i : remainder);
        thread_buckets[i].count = bucket_size + (i < remainder ? 1 : 0);
    }

    auto task_workload = [this](TextureConversionThreadBucket const& threadBucket)
        {
            size_t first = threadBucket.first;
            size_t last = first + threadBucket.count;

            for (size_t i = first; i < last; ++i)
            {
                TextureConversionTask const* task = m_all_conversion_tasks_ordered[i];
                m_texture_upload_tasks[i] = (*task)();
            }
        };

    m_texture_conversion_futures.clear();
    m_texture_conversion_futures.resize(active_threads);
    for (uint32_t i = 0; i < active_threads; ++i)
    {
        m_texture_conversion_futures[i] = std::async(std::launch::async, task_workload, thread_buckets[i]);
    }
}

void TextureConverter::uploadTextures()
{
    waitForTextureConversionCompletion();
    for (auto& texture_upload_task : m_texture_upload_tasks)
    {
        (*texture_upload_task)();
    }

    m_data_uploader.upload();
}

bool TextureConverter::isTextureConversionCompleted() const
{
    return std::all_of(m_texture_conversion_futures.begin(), m_texture_conversion_futures.end(), [](std::future<void> const& e) { return e.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready; });
}

bool TextureConverter::isTextureUploadCompleted() const
{
    return m_data_uploader.isUploadFinished();
}

void TextureConverter::waitForTextureConversionCompletion()
{
    while (!isTextureConversionCompleted())
    {
        std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void TextureConverter::waitForTextureUploadCompletion()
{
    m_data_uploader.waitUntilUploadIsFinished();
}
    

TextureConverter::CachedTextureData TextureConverter::readTextureFromCache(TextureConversionTaskKey const& key, core::misc::UUID& uuid)
{
    size_t valid_blob_size{};
    auto blob_data = m_compressed_textures_cache->retrieveEntry(key, &valid_blob_size);

    dx::d3d12::ResourceDataUploader::TextureSourceDescriptor texture_source_descriptor{};
    size_t blob_data_read_offset{ sha256_provider::c_hash_length };

    {
        uint64_t uuid_hi_part{}, uuid_lo_part{};
        uuid_hi_part = unpackUint64FromArray(static_cast<uint8_t const*>(blob_data.data()), blob_data_read_offset);
        uuid_lo_part = unpackUint64FromArray(static_cast<uint8_t const*>(blob_data.data()), blob_data_read_offset);
        uuid = misc::UUID{ uuid_lo_part, uuid_hi_part };
    }

    conversion::ImageCompressedDataFormat compression_format = static_cast<conversion::ImageCompressedDataFormat>(unpackUint64FromArray(static_cast<uint8_t const*>(blob_data.data()), blob_data_read_offset));

    conversion::ImageLoader::Description image_description{};
    image_description.uri = key.toString();
    image_description.timestamp = m_compressed_textures_cache->getEntryTimestamp(key);

    updateImageDescForcompressionFormat(compression_format, image_description);
    size_t block_size = image_description.element_size;

    bool start_new_layer{ true };
    glm::uvec3 texture_dims_old{ 0, 0, 0 };
    while (blob_data_read_offset < valid_blob_size)
    {
        size_t compressed_img_size{}, compressed_img_row_pitch{}, compressed_img_slice_pitch{};
        compressed_img_size = unpackUint64FromArray(static_cast<uint8_t const*>(blob_data.data()), blob_data_read_offset);
        compressed_img_row_pitch = unpackUint64FromArray(static_cast<uint8_t const*>(blob_data.data()), blob_data_read_offset);
        compressed_img_slice_pitch = unpackUint64FromArray(static_cast<uint8_t const*>(blob_data.data()), blob_data_read_offset);

        size_t texture_width = compressed_img_row_pitch / block_size * 4;
        size_t texture_height = compressed_img_slice_pitch / compressed_img_row_pitch * 4;

        if (start_new_layer)
        {
            image_description.layers.push_back({ .offset = blob_data_read_offset });
            image_description.layers.back().mipmaps.push_back({ .offset = blob_data_read_offset, .dimensions = {texture_width, texture_height, 1} });
            texture_dims_old = { texture_width, texture_height, 1 };
            start_new_layer = false;
        }
        else
        {
            image_description.layers.back().mipmaps.push_back({ .offset = blob_data_read_offset, .dimensions = {texture_width, texture_height, 1} });

            auto dims = image_description.layers.back().mipmaps.back().dimensions;
            if (dims.x == texture_dims_old.x && dims.y == texture_dims_old.y)
            {
                start_new_layer = true;
                texture_dims_old = dims;
            }
        }

        uint8_t* subresource_address = static_cast<uint8_t*>(blob_data.data()) + blob_data_read_offset;
        blob_data_read_offset += compressed_img_size;

        texture_source_descriptor.subresources.push_back({ .p_data = subresource_address, .row_pitch = compressed_img_row_pitch, .slice_pitch = compressed_img_slice_pitch });
    }
    image_description.subresource_count = texture_source_descriptor.subresources.size();

    return { blob_data, image_description, texture_source_descriptor };
}

} 