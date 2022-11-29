#ifndef LEXGINE_SCENEGRAPH_IMAGE_H
#define LEXGINE_SCENEGRAPH_IMAGE_H

#include <vector>
#include <filesystem>
#include <3rd_party/glm/glm.hpp>

#include <engine/core/entity.h>
#include <engine/scenegraph/class_names.h>

namespace lexgine::scenegraph
{

class Image final : public core::NamedEntity<class_names::Image>
{
public:
    struct Mipmap
    {
        size_t offset;
        glm::uvec3 dimensions;    // width and height of the mipmap-level
    };

    struct Layer
    {
        size_t offset;
        std::vector<Mipmap> mipmaps;
    };

public:
    Image(std::filesystem::path const& uri);
    Image(std::vector<uint8_t>&& data, uint32_t width, uint32_t height);

    operator bool() const { return m_valid; }

private:
    void generateMipmaps();

private:
    bool m_valid = false;
    std::vector<uint8_t> m_data;
    std::vector<Layer> m_layers;
    bool m_is_cubemap{ false };
};

}


#endif
