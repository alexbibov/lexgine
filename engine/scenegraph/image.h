#ifndef LEXGINE_SCENEGRAPH_IMAGE_H
#define LEXGINE_SCENEGRAPH_IMAGE_H

#include <vector>
#include <filesystem>
#include <glm/glm.hpp>

#include <engine/core/entity.h>
#include <engine/conversion/lexgine_conversion_fwd.h>
#include <engine/conversion/image_loader.h>
#include <engine/scenegraph/class_names.h>


namespace lexgine::scenegraph
{


class Image final : public core::NamedEntity<class_names::Image>
{
public:
    Image(std::filesystem::path const& uri, conversion::ImageLoaderPool const& image_loader_pool);
    Image(std::vector<uint8_t>&& data, uint32_t width, uint32_t height, size_t element_count, size_t element_size, conversion::ImageColorSpace color_space, std::string const& uri, core::misc::DateTime const& timestamp, conversion::ImageLoaderPool const& image_loader_pool);
    Image(Image&&) = default;
    bool load();

    operator bool() const { return m_valid; }
    std::string const& uri() const { return  m_uri; }

    uint8_t const* data() const { return m_data.data(); }
    uint8_t* data() { return m_data.data(); }
    size_t size() const { return m_data.size(); }

    glm::uvec3 getDimensions() const;
    size_t getLayerCount() const;
    size_t getMipmapCount() const;
    conversion::ImageLoader::Description description() const { return m_description; }

    void generateMipmaps();

private:
    std::string m_uri;
    std::vector<uint8_t> m_data;
    bool m_valid = false;
    conversion::ImageLoaderPool const& m_image_loader_pool;
    conversion::ImageLoader::Description m_description;
};


}


#endif
