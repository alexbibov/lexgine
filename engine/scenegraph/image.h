#ifndef LEXGINE_SCENEGRAPH_IMAGE_H
#define LEXGINE_SCENEGRAPH_IMAGE_H

#include <vector>
#include <filesystem>
#include <3rd_party/glm/glm.hpp>

#include <engine/core/entity.h>
#include <engine/conversion/image_loader.h>
#include <engine/scenegraph/class_names.h>


namespace lexgine::scenegraph
{


class Image final : public core::NamedEntity<class_names::Image>
{
public:
    Image(std::filesystem::path const& uri);
    Image(std::vector<uint8_t>&& data, uint32_t width, uint32_t height, size_t element_count, size_t element_size, conversion::ImageColorSpace color_space, std::string const& uri, core::misc::DateTime const& timestamp);
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

    void registerImageLoader(std::unique_ptr<conversion::ImageLoader>&& image_loader);
    void generateMipmaps();

private:
    std::string m_uri;
    std::vector<uint8_t> m_data;
    bool m_valid = false;
    conversion::ImageLoader::Description m_description;

    std::vector<std::unique_ptr<conversion::ImageLoader>> m_image_loaders;
};


}


#endif
