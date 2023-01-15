#include <cstdlib>  
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION

#define KHRONOS_STATIC
#include <ktx/include/ktx.h>
#undef KHRONOS_STATIC

#include <engine/core/misc/misc.h>
#include "image.h"

#pragma intrinsic (_BitScanReverse)

namespace lexgine::scenegraph
{

namespace {

std::string toLowerCase(std::string const& str)
{
    std::vector<char> aux{};
    std::transform(str.begin(), str.end(), std::back_inserter(aux),
        [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
    return std::string{ aux.begin(), aux.end() };
}

struct KtxIterateLevelsCallbackData final
{
    uint32_t layer;
    std::vector<Image::Layer>& layers;
};


ktx_error_code_e KTX_APIENTRY ktxIterateLevelsCallback(int level, int face, int width, int height, int depth, ktx_uint64_t face_lod_size, void* pixels, void* user_data)
{
    auto callback_data = reinterpret_cast<KtxIterateLevelsCallbackData*>(user_data);
    auto& mipmap = (callback_data->layers)[callback_data->layer].mipmaps[level];
    mipmap.dimensions = { static_cast<unsigned int>(width), static_cast<unsigned int>(height), static_cast<unsigned int>(depth) };

    return KTX_SUCCESS;
}


size_t calculateMipmapCount(size_t base_level_width, size_t base_level_height, size_t base_level_depth)
{
    unsigned long iw{}, ih{}, id{};
    _BitScanReverse(&iw, static_cast<unsigned long>(base_level_width));
    _BitScanReverse(&ih, static_cast<unsigned long>(base_level_height));
    _BitScanReverse(&id, static_cast<unsigned long>(base_level_depth));
    return static_cast<size_t>((std::max)((std::max)(iw, ih), id)) + 1;
}


void createMipmapLevel(std::vector<uint8_t> const& data, size_t element_size, Image::Mipmap const& previous_level_desc, std::vector<uint8_t>& current_level_data)
{
    uint8_t const* source_data_ptr = data.data() + previous_level_desc.offset;
    size_t const row_pitch = previous_level_desc.dimensions.y * element_size;
    size_t const layer_pitch = previous_level_desc.dimensions.x * row_pitch;

    for (uint32_t k = 0; k < previous_level_desc.dimensions.z; k += 2)
    {
        for (uint32_t i = 0; i < previous_level_desc.dimensions.x; i += 2)
        {
            size_t offset = k * layer_pitch + i * row_pitch;
            for (uint32_t j = 0; j < previous_level_desc.dimensions.y; j += 2)
            {
                auto src_data_begin = source_data_ptr + offset + j * element_size;
                current_level_data.insert(current_level_data.end(), src_data_begin, src_data_begin + element_size);
            }
        }
    }
}


}

Image::Image(std::filesystem::path const& uri)
{
    std::string path = uri.string();
    std::string ext = toLowerCase(uri.extension().string());

    auto fetch_data = [&path, this](std::vector<uint8_t>& out)
    {
        auto binary_data = core::misc::readBinaryDataFromSourceFile(path);
        if (!binary_data.isValid())
        {
            LEXGINE_LOG_ERROR(this, "Error while reading image '" + path + "'");
            return false;
        }

        out = std::move(static_cast<std::vector<uint8_t> const&>(binary_data));
        return true;
    };

    if (ext == ".png" || ext == ".jpg")
    {
        std::vector<uint8_t> data{};
        if (!fetch_data(data)) return;


        {
            int width{}, height{}, nchannels{};
            int const req_component = 4;
            m_element_size = req_component;

            auto image_data = stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &nchannels, req_component);
            m_data = std::vector<uint8_t>{ image_data, image_data + width * height * req_component };
            stbi_image_free(image_data);
            m_layers.push_back(Layer{ .offset = 0, .mipmaps = {Mipmap{.offset = 0, .dimensions = {static_cast<unsigned int>(width), static_cast<unsigned int>(height), 1U} }} });
        }
    }
    else if (ext == ".astc")
    {
        LEXGINE_LOG_ERROR(this, "Error while loading image '" + path + "': ASTC compression is not supported");
        return;
    }
    else if (ext == ".ktx" || ext == ".ktx2")
    {
        std::vector<uint8_t> data{};
        if (!fetch_data(data)) return;


        ktxTexture* p_ktx_texture;
        auto ktx_load_result = ktxTexture_CreateFromMemory(static_cast<ktx_uint8_t const*>(data.data()), static_cast<ktx_size_t>(data.size()), KTX_TEXTURE_CREATE_NO_FLAGS, &p_ktx_texture);
        if (ktx_load_result != KTX_SUCCESS)
        {
            LEXGINE_LOG_ERROR(this, "Error while reading KTX image '" + path + "'");
            return;
        }

        if (p_ktx_texture->pData)
        {
            m_data = std::vector<uint8_t>{ p_ktx_texture->pData, p_ktx_texture->pData + p_ktx_texture->dataSize };
        }
        else
        {
            m_data.resize(static_cast<size_t>(p_ktx_texture->dataSize));
            auto ktx_load_data_result = ktxTexture_LoadImageData(p_ktx_texture, m_data.data(), m_data.size());
            if (ktx_load_data_result != KTX_SUCCESS)
            {
                LEXGINE_LOG_ERROR(this, "Error while reading image data from KTX file '" + path + "'");
                return;
            }
        }

        m_element_size = ktxTexture_GetElementSize(p_ktx_texture);

        {
            // Populate mipmap levels and texture layers
            m_is_cubemap = p_ktx_texture->numLayers == 1 && p_ktx_texture->numFaces == 6;
            uint32_t level_count = static_cast<uint32_t>(p_ktx_texture->numLevels);
            if (p_ktx_texture->numLayers > 1 || m_is_cubemap)
            {
                uint32_t layer_count = m_is_cubemap ? static_cast<uint32_t>(p_ktx_texture->numFaces) : static_cast<uint32_t>(p_ktx_texture->numLayers);
                m_layers.resize(layer_count);


                for (uint32_t layer = 0; layer < layer_count; ++layer)
                {
                    m_layers[layer].mipmaps.resize(level_count);
                    ktx_size_t offset{};

                    for (uint32_t level = 0; level < level_count; ++level)
                    {
                        if (m_is_cubemap)
                        {
                            if (auto result = ktxTexture_GetImageOffset(p_ktx_texture, level, 0, layer, &offset); result != KTX_SUCCESS)
                            {
                                LEXGINE_LOG_ERROR(this, "Unable to parse cubemap KTX image level " + std::to_string(level) + ", face " + std::to_string(layer) + " loaded from '" + path + "'");
                                return;
                            }
                        }
                        else
                        {
                            if (auto result = ktxTexture_GetImageOffset(p_ktx_texture, level, layer, 0, &offset); result != KTX_SUCCESS)
                            {
                                LEXGINE_LOG_ERROR(this, "Unable to parse KTX image level " + std::to_string(level) + ", layer " + std::to_string(layer) + " loaded from '" + path + "'");
                                return;
                            }
                        }

                        if (level == 0)
                        {
                            m_layers[layer].offset = offset;
                        }

                        auto& mipmap = m_layers[layer].mipmaps[level];
                        mipmap.offset = offset;


                        KtxIterateLevelsCallbackData callback_data{ layer, m_layers };
                        if (auto result = ktxTexture_IterateLevels(p_ktx_texture, ktxIterateLevelsCallback, &callback_data); result != KTX_SUCCESS)
                        {
                            LEXGINE_LOG_ERROR(this, "Unable to iterate over mipmap levels in layer " + std::to_string(layer) + " for KTX texture '" + path + "'");
                            return;
                        }
                    }
                }
            }
            else
            {
                m_layers.push_back({ 0, {} });
                auto& mipmap_levels = m_layers.back().mipmaps;
                mipmap_levels.resize(level_count);

                for (uint32_t level = 0; level < level_count; ++level)
                {
                    auto& mipmap = mipmap_levels[level];

                    ktx_size_t offset{};
                    if (auto result = ktxTexture_GetImageOffset(p_ktx_texture, level, 0, 0, &offset); result != KTX_SUCCESS)
                    {
                        LEXGINE_LOG_ERROR(this, "Unable to parse KTX image level " + std::to_string(level) + " loaded from '" + path + "'");
                        return;
                    }
                }

                KtxIterateLevelsCallbackData callback_data{ 0, m_layers };
                if (auto result = ktxTexture_IterateLevels(p_ktx_texture, ktxIterateLevelsCallback, &callback_data); result != KTX_SUCCESS)
                {
                    LEXGINE_LOG_ERROR(this, "Unable to iterate over mipmap levels in KTX texture loaded from '" + path + "'");
                    return;
                }
            }
        }


        ktxTexture_Destroy(p_ktx_texture);
    }
    else {
        LEXGINE_LOG_ERROR(this, "Error while loading image '" + path + "': unsupported format");
        return;
    }

    generateMipmaps();
    m_valid = true;
}


Image::Image(std::vector<uint8_t>&& data, uint32_t width, uint32_t height)
    : m_data{ std::move(data) }
{
    m_layers.push_back(Layer{ .offset = 0, .mipmaps = {Mipmap{.offset = 0, .dimensions = {width, height, 1U}} } });
    generateMipmaps();
    m_valid = true;
}


void Image::generateMipmaps()
{
    std::vector<uint8_t> generated_data{};
    for (auto& e : m_layers)
    {
        auto last_mipmap_level_dimensions = e.mipmaps.back().dimensions;
        size_t missing_mipmap_count = calculateMipmapCount(last_mipmap_level_dimensions.x, last_mipmap_level_dimensions.y, last_mipmap_level_dimensions.z) - 1;
        for (size_t i = 0; i < missing_mipmap_count; ++i)
        {
            last_mipmap_level_dimensions = {
                (std::max)(last_mipmap_level_dimensions.x >> 1, 1u),
                (std::max)(last_mipmap_level_dimensions.y >> 1, 1u),
                (std::max)(last_mipmap_level_dimensions.z >> 1, 1u)
            };

            Mipmap level_desc{ m_data.size() + generated_data.size(), last_mipmap_level_dimensions };
            createMipmapLevel(m_data, m_element_size, e.mipmaps.back(), generated_data);
            e.mipmaps.push_back(level_desc);
        }
    }

    m_data.insert(m_data.end(), generated_data.begin(), generated_data.end());
}


}
