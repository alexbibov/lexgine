#include <cstdlib>  
#include <cassert>
#include <array>
#include <engine/core/misc/misc.h>

#include "image.h"

#pragma intrinsic (_BitScanReverse)

namespace lexgine::scenegraph
{

namespace {

size_t calculateMipmapCount(size_t base_level_width, size_t base_level_height, size_t base_level_depth)
{
    unsigned long iw{}, ih{}, id{};
    _BitScanReverse(&iw, static_cast<unsigned long>(base_level_width));
    _BitScanReverse(&ih, static_cast<unsigned long>(base_level_height));
    _BitScanReverse(&id, static_cast<unsigned long>(base_level_depth));
    return static_cast<size_t>((std::max)((std::max)(iw, ih), id)) + 1;
}

void createMipmapLevel(std::vector<uint8_t>& texture_buffer, size_t texel_size, conversion::ImageLoader::Mipmap const& base_level_desc, uint32_t order,
    size_t* p_new_mipmap_height, size_t* p_new_mipmap_width, size_t* p_new_mipmap_depth, size_t* p_new_mipmap_offset)
{
    size_t const row_pitch = base_level_desc.dimensions.x * texel_size;
    size_t const layer_pitch = base_level_desc.dimensions.y * row_pitch;

    size_t mipmap_height{}, mipmap_width{}, mipmap_depth{};
    *p_new_mipmap_height = mipmap_height = (std::max)(base_level_desc.dimensions.x >> order, 1U);
    *p_new_mipmap_width = mipmap_width = (std::max)(base_level_desc.dimensions.y >> order, 1U);
    *p_new_mipmap_depth = mipmap_depth = (std::max)(base_level_desc.dimensions.z >> order, 1U);
    size_t mipmap_size = mipmap_width * mipmap_height * mipmap_depth * texel_size;

    size_t const new_level_row_pitch = mipmap_width * texel_size;
    size_t const new_level_layer_pitch = mipmap_height * new_level_row_pitch;
    size_t const new_level_base_offset = *p_new_mipmap_offset = base_level_desc.offset + layer_pitch * base_level_desc.dimensions.z;
    for (uint32_t k = 0; k < base_level_desc.dimensions.z; k += (2 << order - 1))
    {
        size_t layer_offset{ k * layer_pitch };
        size_t new_lvl_layer_offset{ k / 2 * new_level_layer_pitch };
        for (uint32_t j = 0; j < base_level_desc.dimensions.y; j += (2 << order - 1))
        {
            size_t row_offset{ layer_offset + j * row_pitch };
            size_t new_lvl_row_offset{ new_lvl_layer_offset + j / 2 * row_pitch };
            for (uint32_t i = 0; i < base_level_desc.dimensions.x; i += (2 << order - 1))
            {
                texture_buffer[new_level_base_offset + new_lvl_row_offset + i / 2] = texture_buffer[base_level_desc.offset + row_offset + i];
            }
        }
    }
}

} // namespace


Image::Image(std::filesystem::path const& uri)
    : m_uri{ uri.string() }
    , m_image_loaders{}
{
    
}


Image::Image(std::vector<uint8_t>&& data, uint32_t width, uint32_t height, size_t element_count, size_t element_size, conversion::ImageColorSpace color_space, std::string const& uri, core::misc::DateTime const& timestamp)
    : m_uri{ uri }
    , m_data{ std::move(data) }
{
    m_description.timestamp = timestamp;
    m_description.element_count = element_count;
    m_description.element_size = element_size;
    m_description.color_space = color_space;
    m_description.is_unsigned = color_space != conversion::ImageColorSpace::none;
    m_description.compression_format = conversion::ImageCompressedDataFormat::no_compression;
    m_description.layers.push_back(conversion::ImageLoader::Layer{ .offset = 0, .mipmaps = {conversion::ImageLoader::Mipmap{.offset = 0, .dimensions = {width, height, 1U}} } });
    m_description.is_cubemap = false;
    m_description.subresource_count = 1;
    m_valid = true;
}

bool Image::load()
{
    if (m_valid)
        return true;

    std::filesystem::path p{m_uri};
    for (auto const& e : m_image_loaders)
    {
        if (e->canLoad(p))
        {
            m_valid = e->load(p, m_data);
            if (m_valid)
            {
                m_description = e->description();
                break;
            }
        }
    }

    return m_valid;
}

glm::uvec3 Image::getDimensions() const
{
    auto& base_mipmap_level = m_description.layers[0].mipmaps[0];
    return glm::uvec3{ base_mipmap_level.dimensions };
}

size_t Image::getLayerCount() const
{
    return m_description.layers.size();
}

size_t Image::getMipmapCount() const
{
    return m_description.layers[0].mipmaps.size();
}

void Image::registerImageLoader(std::unique_ptr<conversion::ImageLoader>&& image_loader)
{
    m_image_loaders.emplace_back(std::move(image_loader));
}

void Image::generateMipmaps()
{
    size_t texel_size = m_description.element_size * m_description.element_count;
    for (auto& e : m_description.layers)
    {
        conversion::ImageLoader::Mipmap const& last_loaded_mipmap = e.mipmaps.back();
        size_t missing_mipmap_count = calculateMipmapCount(last_loaded_mipmap.dimensions.x, last_loaded_mipmap.dimensions.y, last_loaded_mipmap.dimensions.z) - 1;
        m_description.subresource_count += missing_mipmap_count;
        for (size_t i = 0; i < missing_mipmap_count; ++i)
        {
            size_t new_mipmap_height{}, new_mipmap_width{}, new_mipmap_depth{}, new_mipmap_offset{};
            createMipmapLevel(m_data, texel_size, last_loaded_mipmap, i + 1, &new_mipmap_height, &new_mipmap_width, &new_mipmap_depth, &new_mipmap_offset);
            e.mipmaps.push_back(conversion::ImageLoader::Mipmap{ new_mipmap_offset, glm::uvec3{new_mipmap_width, new_mipmap_height, new_mipmap_depth} });
        }
    }
}


}
