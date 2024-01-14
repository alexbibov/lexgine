#ifndef LEXGINE_CONVERSION_KTX_IMAGE_LOADER_H
#define LEXGINE_CONVERSION_KTX_IMAGE_LOADER_H

#include "image_loader.h"

namespace lexgine::conversion
{

class KtxImageLoader final : public ImageLoader
{
public:
    KtxImageLoader() = default;
    bool canLoad(std::filesystem::path const& uri) const override;
private:
    bool doLoad(std::vector<uint8_t> const& raw_binary_data, std::vector<uint8_t>& image_data_buffer) override;
};

}

#endif