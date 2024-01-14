#include <cstdint>

#define KHRONOS_STATIC
#include <ktx/include/ktx.h>
extern "C" {
    ktx_uint32_t ktxTexture1_glTypeSize(ktxTexture1* This);
}
#undef KHRONOS_STATIC

#include "ktx_image_loader.h"

namespace lexgine::conversion
{

namespace
{

ktx_error_code_e KTX_APIENTRY ktxIterateLevelsCallback(int level, int face, int width, int height, int depth, ktx_uint64_t face_lod_size, void* pixels, void* user_data)
{
    auto& mipmaps = *reinterpret_cast<std::vector<ImageLoader::Mipmap>*>(user_data);
    auto& mipmap_level = mipmaps[level];
    mipmap_level.dimensions = { static_cast<unsigned int>(width), static_cast<unsigned int>(height), static_cast<unsigned int>(depth) };

    return KTX_SUCCESS;
}

size_t ktxTexture_GetElementCount(ktxTexture* p_ktx_texture)
{
    if (p_ktx_texture->classId == class_id::ktxTexture1_c)
    {
        // KTX1 texture
        ktx_uint32_t type_size = ktxTexture1_glTypeSize(reinterpret_cast<ktxTexture1*>(p_ktx_texture));
        ktx_uint32_t texel_size = ktxTexture_GetElementSize(p_ktx_texture);
        return texel_size / type_size;
    }
    else if (p_ktx_texture->classId == class_id::ktxTexture2_c)
    {
        // KTX2 texture
        return ktxTexture2_GetNumComponents(reinterpret_cast<ktxTexture2*>(p_ktx_texture));
    }

    return 0;
}

ImageCompressedDataFormat ktxTexture_GetCompressionFormat(ktxTexture* p_ktx_texture)
{
    if (!p_ktx_texture->isCompressed) return ImageCompressedDataFormat::no_compression;

    if (p_ktx_texture->classId == class_id::ktxTexture2_c)
    {
        ktxTexture2* p_ktx_texture2 = reinterpret_cast<ktxTexture2*>(p_ktx_texture);
        switch (p_ktx_texture2->vkFormat)
        {
        case /*VK_FORMAT_BC1_RGBA_UNORM_BLOCK = */ 133:
            return ImageCompressedDataFormat::bc1_unorm;

        case /*VK_FORMAT_BC1_RGBA_SRGB_BLOCK = */ 134:
            return ImageCompressedDataFormat::bc1_unorm_srgb;

        case /*VK_FORMAT_BC2_UNORM_BLOCK = */ 135:
            return ImageCompressedDataFormat::bc2_unorm;

        case /*VK_FORMAT_BC2_SRGB_BLOCK = */ 136:
            return ImageCompressedDataFormat::bc2_unorm_srgb;

        case /*VK_FORMAT_BC3_UNORM_BLOCK = */ 137:
            return ImageCompressedDataFormat::bc3_unorm;

        case /*VK_FORMAT_BC3_SRGB_BLOCK = */ 138:
            return ImageCompressedDataFormat::bc3_unorm_srgb;

        case /*VK_FORMAT_BC4_UNORM_BLOCK = */ 139:
            return ImageCompressedDataFormat::bc4_unorm;

        case /*VK_FORMAT_BC4_SNORM_BLOCK = */ 140:
            return ImageCompressedDataFormat::bc4_snorm;

        case /*VK_FORMAT_BC5_UNORM_BLOCK = */ 141:
            return ImageCompressedDataFormat::bc5_unorm;

        case /*VK_FORMAT_BC5_SNORM_BLOCK = */ 142:
            return ImageCompressedDataFormat::bc5_snorm;

        case /*VK_FORMAT_BC6H_UFLOAT_BLOCK = */ 143:
            return ImageCompressedDataFormat::bc6h_uf16;

        case /*VK_FORMAT_BC6H_SFLOAT_BLOCK = */ 144:
            return ImageCompressedDataFormat::bc6h_sf16;

        case /*VK_FORMAT_BC7_UNORM_BLOCK = */ 145:
            return ImageCompressedDataFormat::bc7_unorm;

        case /*VK_FORMAT_BC7_SRGB_BLOCK = */ 146:
            return ImageCompressedDataFormat::bc7_unorm_srgb;
        }
    }

    // Compression in KTX1 textures is not supported
    return ImageCompressedDataFormat::unknown;
}

}  // namespace


bool KtxImageLoader::canLoad(std::filesystem::path const& uri) const
{
    std::string ext = core::misc::toLowerCase(uri.extension().string());
    return ext == ".ktx";
}

bool KtxImageLoader::doLoad(std::vector<uint8_t> const& raw_binary_data, std::vector<uint8_t>& image_data_buffer)
{
    ktxTexture* p_ktx_texture;
    auto ktx_load_result = ktxTexture_CreateFromMemory(static_cast<ktx_uint8_t const*>(raw_binary_data.data()), static_cast<ktx_size_t>(raw_binary_data.size()), KTX_TEXTURE_CREATE_NO_FLAGS, &p_ktx_texture);
    if (ktx_load_result != KTX_SUCCESS)
    {
        LEXGINE_LOG_ERROR(this, "Error while reading KTX image '" + m_description.uri + "'");
        return false;
    }


    size_t num_layers{}, num_levels{ p_ktx_texture->numLevels }, ktx_texture_data_size{ static_cast<size_t>(p_ktx_texture->dataSize) };
    {
        // fill in texture metadata
        m_description.is_cubemap = p_ktx_texture->numLayers == 1 && p_ktx_texture->numFaces == 6;
        num_layers = m_description.is_cubemap ? 6 : p_ktx_texture->numLayers;
        auto texel_size = ktxTexture_GetElementSize(p_ktx_texture);
        m_description.element_count = ktxTexture_GetElementCount(p_ktx_texture);
        m_description.element_size = texel_size / m_description.element_count;
        m_description.compression_format = ktxTexture_GetCompressionFormat(p_ktx_texture);
        m_description.subresource_count = num_layers * num_levels;

        if (m_description.compression_format == ImageCompressedDataFormat::unknown)
        {
            LEXGINE_LOG_ERROR(this, "Unable to parse KTX texture: the texture appears to be compressed, but its compression format is not supported");
            return false;
        }

        if (m_description.element_count < 1 || m_description.element_count > 4)
        {
            LEXGINE_LOG_ERROR(this, "Unable to parse KTX texture: the element size of " + std::to_string(m_description.element_count) + "is not supported");
            return false;
        }

        if (m_description.element_count == 4 || m_description.element_count == 3) {
            m_description.color_space = ImageColorSpace::srgb;
        }
    }


    {
        // Load data

        std::vector<uint8_t> scratch_buffer(ktx_texture_data_size);
        uint8_t* p_src_data_buffer{ nullptr };
        if (p_ktx_texture->pData)
        {
            p_src_data_buffer = p_ktx_texture->pData;
        }
        else
        {
            auto ktx_load_data_result = ktxTexture_LoadImageData(p_ktx_texture, scratch_buffer.data(), ktx_texture_data_size);
            if (ktx_load_data_result != KTX_SUCCESS)
            {
                LEXGINE_LOG_ERROR(this, "Error while reading image data from KTX file '" + m_description.uri + "'");
                return false;
            }
            p_src_data_buffer = scratch_buffer.data();
        }

        m_description.layers.resize(num_layers);
        size_t width4{}, height4{}, pyramid_size{};
        for (uint32_t layer = 0; layer < num_layers; ++layer)
        {
            auto& mipmaps = m_description.layers[layer].mipmaps;
            mipmaps.resize(num_levels);
            if (auto result = ktxTexture_IterateLevels(p_ktx_texture, ktxIterateLevelsCallback, &mipmaps); result != KTX_SUCCESS)
            {
                LEXGINE_LOG_ERROR(this, "Unable to iterate over mipmap levels in layer " + std::to_string(layer) + " for KTX texture '" + m_description.uri + "'");
                return false;
            }


            size_t dst_offset{ pyramid_size * m_description.element_count * layer };
            size_t aligned_width{}, alighned_height{};
            for (uint32_t level = 0; level < num_levels; ++level)
            {
                ktx_size_t src_offset{};
                auto& target_mipmap_lvl = mipmaps[level];

                if (layer == 0 && level == 0)
                {
                    // Note that when texture is compressed, the dimension rounding is not required and hence, is a no-op (see implementation of roundToNextMultipleOf4(...))
                    aligned_width = target_mipmap_lvl.dimensions.x;
                    alighned_height = target_mipmap_lvl.dimensions.y;
                    width4 = roundToNextMultipleOf4(target_mipmap_lvl.dimensions.x, m_description.compression_format);
                    height4 = roundToNextMultipleOf4(target_mipmap_lvl.dimensions.y, m_description.compression_format);
                    pyramid_size = calculateMipmapPyramidCapacity(width4, height4, target_mipmap_lvl.dimensions.z);
                    image_data_buffer.resize(pyramid_size * m_description.element_count * num_layers);
                }

                if (m_description.is_cubemap)
                {
                    if (auto result = ktxTexture_GetImageOffset(p_ktx_texture, level, 0, layer, &src_offset); result != KTX_SUCCESS)
                    {
                        LEXGINE_LOG_ERROR(this, "Unable to parse cubemap KTX image level " + std::to_string(level) + ", face " + std::to_string(layer) + " loaded from '" + m_description.uri + "'");
                        return false;
                    }
                }
                else
                {
                    if (auto result = ktxTexture_GetImageOffset(p_ktx_texture, level, layer, 0, &src_offset); result != KTX_SUCCESS)
                    {
                        LEXGINE_LOG_ERROR(this, "Unable to parse KTX image level " + std::to_string(level) + ", layer " + std::to_string(layer) + " loaded from '" + m_description.uri + "'");
                        return false;
                    }
                }


                if (aligned_width != target_mipmap_lvl.dimensions.x || alighned_height != target_mipmap_lvl.dimensions.y)
                {
                    // Current mipmap level has to be resized
                    switch (m_description.element_count)
                    {
                    case 1:
                        resizeImage1(p_src_data_buffer + src_offset, target_mipmap_lvl.dimensions.x, target_mipmap_lvl.dimensions.y, target_mipmap_lvl.dimensions.z,
                            aligned_width, alighned_height, image_data_buffer.data() + dst_offset);
                        break;

                    case 2:
                        resizeImage2(p_src_data_buffer + src_offset, target_mipmap_lvl.dimensions.x, target_mipmap_lvl.dimensions.y, target_mipmap_lvl.dimensions.z,
                            aligned_width, alighned_height, image_data_buffer.data() + dst_offset);
                        break;

                    case 3:
                        resizeImage3(p_src_data_buffer + src_offset, target_mipmap_lvl.dimensions.x, target_mipmap_lvl.dimensions.y, target_mipmap_lvl.dimensions.z,
                            aligned_width, alighned_height, image_data_buffer.data() + dst_offset);
                        break;

                    case 4:
                        resizeImage4(p_src_data_buffer + src_offset, target_mipmap_lvl.dimensions.x, target_mipmap_lvl.dimensions.y, target_mipmap_lvl.dimensions.z,
                            aligned_width, alighned_height, image_data_buffer.data() + dst_offset);
                        break;
                    }
                }
                else
                {
                    // No resize is needed, just copy the data
                    std::copy(
                        p_src_data_buffer + src_offset,
                        p_src_data_buffer + src_offset + target_mipmap_lvl.dimensions.x * target_mipmap_lvl.dimensions.y * m_description.element_count,
                        image_data_buffer.begin() + dst_offset
                    );
                }


                if (level == 0)
                {
                    m_description.layers[layer].offset = dst_offset;
                }

                auto& mipmap = m_description.layers[layer].mipmaps[level];
                mipmap.offset = dst_offset;

                dst_offset += aligned_width * alighned_height * m_description.element_count;
                aligned_width >>= 1;
                alighned_height >>= 1;
            }
        }

    }


    ktxTexture_Destroy(p_ktx_texture);

    return true;
}

}