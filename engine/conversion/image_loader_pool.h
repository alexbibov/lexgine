#ifndef LEXGINE_CONVERSION_IMAGE_LOADER_POOL_H

#include "image_loader.h"

namespace lexgine::conversion {

class ImageLoaderPool 
{
    using pool = std::vector<std::unique_ptr<conversion::ImageLoader>>;

public:
    ImageLoaderPool();
    void registerImageLoader(std::unique_ptr<conversion::ImageLoader>&& image_loader);

    pool::const_iterator begin() const { return m_image_loaders.cbegin();}
    pool::const_iterator end() const { return m_image_loaders.cend(); }

private:
    pool m_image_loaders;
};

}

#endif