#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION

#include <engine/core/misc/misc.h>
#include "png_jpg_image_loader.h"

namespace lexgine::conversion
{

bool PNGJPGImageLoader::canLoad(std::filesystem::path const& uri) const
{
    std::string ext = core::misc::toLowerCase(uri.extension().string());
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
}

bool PNGJPGImageLoader::doLoad(std::vector<uint8_t> const& raw_binary_data, std::vector<uint8_t>& image_data_buffer)
{
    int width{}, height{}, nchannels{};
    int const req_component = 4;
    m_description.element_count = req_component;
    m_description.element_size = 1;
    m_description.color_space = ImageColorSpace::rgb;
    m_description.compression_format = ImageCompressedDataFormat::no_compression;
    m_description.is_unsigned = true;

    auto image_data = stbi_load_from_memory(raw_binary_data.data(), static_cast<int>(raw_binary_data.size()), &width, &height, &nchannels, req_component);
    uint32_t width4 = roundToNextMultipleOf4(static_cast<uint32_t>(width), m_description.compression_format);
    uint32_t height4 = roundToNextMultipleOf4(static_cast<uint32_t>(height), m_description.compression_format);

    image_data_buffer.resize(calculateMipmapPyramidCapacity(width4, height4, 1) * m_description.element_count);
    if (width4 != width || height4 != height)
    {
        resizeImage4(static_cast<uint8_t*>(image_data), static_cast<size_t>(width), static_cast<size_t>(height), 1, width4, height4, image_data_buffer.data());
    }
    else
    {
        std::copy(image_data, image_data + width * height * req_component, image_data_buffer.data());
    }

    stbi_image_free(image_data);

    m_description.layers.push_back(Layer{ .offset = 0, .mipmaps = {Mipmap{.offset = 0, .dimensions = {static_cast<unsigned int>(width), static_cast<unsigned int>(height), 1U} }} });
    m_description.subresource_count = 1;

    return true;
}

}