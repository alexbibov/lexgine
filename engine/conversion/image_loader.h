#ifndef LEXGINE_CONVERSION_IMAGE_LOADER_H
#define LEXGINE_CONVERSION_IMAGE_LOADER_H

#include <filesystem>
#include <glm/glm.hpp>
#include <engine/core/entity.h>
#include <engine/core/misc/optional.h>
#include "class_names.h"

namespace lexgine::conversion
{

enum class ImageColorSpace
{
    rgb, srgb, hdr, none
};


enum class ImageCompressedDataFormat : uint32_t
{
    bc1_unorm = DXGI_FORMAT_BC1_UNORM,
    bc1_unorm_srgb = DXGI_FORMAT_BC1_UNORM_SRGB,
    bc2_unorm = DXGI_FORMAT_BC2_UNORM,
    bc2_unorm_srgb = DXGI_FORMAT_BC2_UNORM_SRGB,
    bc3_unorm = DXGI_FORMAT_BC3_UNORM,
    bc3_unorm_srgb = DXGI_FORMAT_BC3_UNORM_SRGB,
    bc4_unorm = DXGI_FORMAT_BC4_UNORM,
    bc4_snorm = DXGI_FORMAT_BC4_SNORM,
    bc5_unorm = DXGI_FORMAT_BC5_UNORM,
    bc5_snorm = DXGI_FORMAT_BC5_SNORM,
    bc6h_uf16 = DXGI_FORMAT_BC6H_UF16,
    bc6h_sf16 = DXGI_FORMAT_BC6H_SF16,
    bc7_unorm = DXGI_FORMAT_BC7_UNORM,
    bc7_unorm_srgb = DXGI_FORMAT_BC7_UNORM_SRGB,
    unknown = DXGI_FORMAT_UNKNOWN,
    no_compression = static_cast<uint32_t>(-1)
};


class ImageLoader : public core::NamedEntity<class_names::ImageLoader>
{
public:
    struct Mipmap
    {
        size_t offset;
        glm::uvec3 dimensions;    // width, height, and depth of the mipmap-level
    };

    struct Layer
    {
        size_t offset;
        std::vector<Mipmap> mipmaps;
    };

    struct Description
    {
        std::string uri;
        ImageColorSpace color_space;
        core::misc::DateTime timestamp;
        uint8_t element_count;
        uint8_t element_size;
        bool is_unsigned;
        ImageCompressedDataFormat compression_format;
        std::vector<Layer> layers;
        bool is_cubemap;
        size_t subresource_count;
    };

public:
    virtual ~ImageLoader() = default;

    virtual bool canLoad(std::filesystem::path const& uri) const = 0;
    bool load(std::filesystem::path const& uri, std::vector<uint8_t>& image_data_buffer);
    Description description() const { return m_description; }

protected:
    virtual bool doLoad(std::vector<uint8_t> const& raw_binary_data, std::vector<uint8_t>& image_data_buffer) = 0;
    
    // Internal utility functions
    uint32_t roundToNextMultipleOf4(uint32_t x, ImageCompressedDataFormat compression)
    {
        return compression == ImageCompressedDataFormat::no_compression ? x + ((4 - (x & 3)) & 3) : x;
    }

    static size_t calculateMipmapPyramidCapacity(size_t base_level_width, size_t base_level_height, size_t base_level_depth);
    static void resizeImage1(uint8_t const* src_image_data, size_t src_image_width, size_t src_image_height, size_t src_image_depth, size_t target_width, size_t target_height, uint8_t* output_buffer);
    static void resizeImage2(uint8_t const* src_image_data, size_t src_image_width, size_t src_image_height, size_t src_image_depth, size_t target_width, size_t target_height, uint8_t* output_buffer);
    static void resizeImage3(uint8_t const* src_image_data, size_t src_image_width, size_t src_image_height, size_t src_image_depth, size_t target_width, size_t target_height, uint8_t* output_buffer);
    static void resizeImage4(uint8_t const* src_image_data, size_t src_image_width, size_t src_image_height, size_t src_image_depth, size_t target_width, size_t target_height, uint8_t* output_buffer);


protected:
    Description m_description;
};

}

#endif