#include "ktx_image_loader.h"
#include "png_jpg_image_loader.h"

#include "image_loader_pool.h"


namespace lexgine::conversion {

ImageLoaderPool::ImageLoaderPool()
{
    registerImageLoader(std::make_unique<KtxImageLoader>());
    registerImageLoader(std::make_unique<PNGJPGImageLoader>());
}

void ImageLoaderPool::registerImageLoader(std::unique_ptr<ImageLoader>&& image_loader)
{
    m_image_loaders.emplace_back(std::move(image_loader));
}

}