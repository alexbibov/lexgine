#include <engine/core/misc/misc.h>
#include "image_loader.h"

namespace lexgine::conversion
{

namespace
{
core::misc::DateTime getTimestampForUri(std::filesystem::path const& uri)
{
    auto timestamp = core::misc::getFileLastUpdatedTimeStamp(uri.string());
    return timestamp.isValid() ? *timestamp : core::misc::DateTime::buildTime();
}


template<uint8_t element_count> struct FSReturnType {};
template<> struct FSReturnType<1>
{
    using fetch_value_type = glm::u8vec1;
    using sample_value_type = glm::vec1;
};

template<> struct FSReturnType<2>
{
    using fetch_value_type = glm::u8vec2;
    using sample_value_type = glm::vec2;
};

template<> struct FSReturnType<3>
{
    using fetch_value_type = glm::u8vec3;
    using sample_value_type = glm::vec3;
};

template<> struct FSReturnType<4>
{
    using fetch_value_type = glm::u8vec4;
    using sample_value_type = glm::vec4;
};

template<uint8_t element_count>
FSReturnType<element_count>::fetch_value_type fetch(uint8_t const* image_data, size_t image_width, size_t image_height, size_t image_depth, glm::uvec3 const& fetch_position)
{
    size_t const offset = ((fetch_position.z * image_height + fetch_position.y) * image_width + fetch_position.x) * element_count;
    typename FSReturnType<element_count>::fetch_value_type rv{};
    for (uint8_t i = 0; i < element_count; ++i)
    {
        rv[i] = image_data[offset + i];
    }
    return rv;
}

template<uint8_t element_count>
typename FSReturnType<element_count>::sample_value_type sample(uint8_t const* image_data, size_t image_width, size_t image_height, size_t image_depth, glm::vec2 const& uv_position, size_t depth_layer)
{
    glm::vec2 pixel_position = uv_position * glm::vec2{ image_width - 1.f, image_height - 1.f };
    glm::uvec2 pp00 = static_cast<glm::uvec2>(glm::floor(pixel_position));
    glm::uvec2 pp11 = static_cast<glm::uvec2>(glm::ceil(pixel_position));
    glm::uvec2 pp01{ pp00.x, pp11.y };
    glm::uvec2 pp10{ pp11.x, pp00.y };


    auto i00 = fetch<element_count>(image_data, image_width, image_height, image_depth, glm::uvec3{ pp00, static_cast<unsigned int>(depth_layer) });
    auto i01 = fetch<element_count>(image_data, image_width, image_height, image_depth, glm::uvec3{ pp01, static_cast<unsigned int>(depth_layer) });
    auto i10 = fetch<element_count>(image_data, image_width, image_height, image_depth, glm::uvec3{ pp10, static_cast<unsigned int>(depth_layer) });
    auto i11 = fetch<element_count>(image_data, image_width, image_height, image_depth, glm::uvec3{ pp11, static_cast<unsigned int>(depth_layer) });

    float x_fract = pixel_position.x - pp00.x;
    float y_fract = pixel_position.y - pp00.y;

    auto interpX0 = glm::mix(i00, i10, x_fract);
    auto interpX1 = glm::mix(i01, i11, x_fract);

    return glm::mix(interpX0, interpX1, y_fract);
}

template<uint8_t element_count>
void resizeImage(uint8_t const* src_image_data, size_t src_image_width, size_t src_image_height, size_t src_image_depth, size_t target_width, size_t target_height, uint8_t* output_buffer)
{
    assert(!(src_image_width == target_width && src_image_height == target_height));

    for (size_t depth_layer = 0; depth_layer < src_image_depth; ++depth_layer) {
        size_t layer_offset = src_image_height * depth_layer;
        for (size_t i = 0; i < src_image_height; ++i) {
            size_t row_offset = (layer_offset + i) * src_image_width;
            for (size_t j = 0; j < src_image_width; ++j) {
                size_t pixel_offset = (row_offset + j) * element_count;
                glm::vec2 uv_position{ i / (src_image_height - 1.f), j / (src_image_width - 1.f) };
                auto val = sample<element_count>(src_image_data, src_image_width, src_image_height, src_image_depth, uv_position, depth_layer);
                for (uint8_t k = 0; k < element_count; ++k) {
                    output_buffer[pixel_offset + k] = val[k];
                }
            }
        }
    }
}

}

bool ImageLoader::load(std::filesystem::path const& uri, std::vector<uint8_t>& image_data_buffer)
{
    auto binary_data = core::misc::readBinaryDataFromSourceFile(uri.string());
    if (!binary_data.isValid())
    {
        LEXGINE_LOG_ERROR(this, "Error while reading image '" + uri.string() + "'");
        return false;
    }
    m_description.uri = uri.string();
    m_description.timestamp = getTimestampForUri(uri);
    bool res = doLoad(*binary_data, image_data_buffer);
    if (res)
    {
        glm::uvec3& dims = m_description.layers[0].mipmaps[0].dimensions;
        size_t texel_size = m_description.element_count * m_description.element_size;
        image_data_buffer.resize(calculateMipmapPyramidCapacity(dims.x, dims.y, dims.z) * texel_size);
    }
    return res;
}

inline unsigned int pow2(unsigned int value, unsigned int power)
{
    return 1 << value * power;
}

size_t ImageLoader::calculateMipmapPyramidCapacity(size_t base_level_width, size_t base_level_height, size_t base_level_depth)
{
    size_t rv{ 0 };
    while (base_level_width > 1 || base_level_height > 1 || base_level_depth > 1)
    {
        rv += base_level_width * base_level_height * base_level_depth;
        base_level_width = (std::max)(base_level_width >> 1, static_cast<size_t>(1));
        base_level_height = (std::max)(base_level_height >> 1, static_cast<size_t>(1));
        base_level_depth = (std::max)(base_level_depth >> 1, static_cast<size_t>(1));
    }

    return rv + 1;
}

void ImageLoader::resizeImage1(uint8_t const* src_image_data, size_t src_image_width, size_t src_image_height, size_t src_image_depth, size_t target_width, size_t target_height, uint8_t* output_buffer)
{
    resizeImage<1>(src_image_data, src_image_width, src_image_height, src_image_depth, target_width, target_height, output_buffer);
}

void ImageLoader::resizeImage2(uint8_t const* src_image_data, size_t src_image_width, size_t src_image_height, size_t src_image_depth, size_t target_width, size_t target_height, uint8_t* output_buffer)
{
    resizeImage<2>(src_image_data, src_image_width, src_image_height, src_image_depth, target_width, target_height, output_buffer);
}

void ImageLoader::resizeImage3(uint8_t const* src_image_data, size_t src_image_width, size_t src_image_height, size_t src_image_depth, size_t target_width, size_t target_height, uint8_t* output_buffer)
{
    resizeImage<3>(src_image_data, src_image_width, src_image_height, src_image_depth, target_width, target_height, output_buffer);
}

void ImageLoader::resizeImage4(uint8_t const* src_image_data, size_t src_image_width, size_t src_image_height, size_t src_image_depth, size_t target_width, size_t target_height, uint8_t* output_buffer)
{
    resizeImage<4>(src_image_data, src_image_width, src_image_height, src_image_depth, target_width, target_height, output_buffer);
}

}