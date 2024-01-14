#ifndef LEXGINE_CONVERSION_PNG_JPG_IMAGE_LOADER_H
#define LEXGINE_CONVERSION_PNG_JPG_IMAGE_LOADER_H

#include "image_loader.h"

namespace lexgine::conversion
{

class PNGJPGImageLoader final : public ImageLoader
{
public:
    bool canLoad(std::filesystem::path const& uri) const override;

private:
    bool doLoad(std::vector<uint8_t> const& raw_binary_data, std::vector<uint8_t>& loaded_image_data_buffer) override;
};

}

#endif