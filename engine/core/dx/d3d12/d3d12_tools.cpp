#include <algorithm>
#include <array>

#include <engine/core/vertex_attributes.h>
#include <engine/core/misc/misc.h>
#include <engine/core/dx/d3d12/d3d12_tools.h>
#include <engine/core/misc/static_template_iterator.h>

namespace lexgine::core::dx::d3d12 {

bool DxgiFormatFetcher::format_desc::operator==(format_desc const& other) const
{
    return is_fp == other.is_fp && is_signed == other.is_signed
        && is_normalized == other.is_normalized && element_count == other.element_count
        && element_size == other.element_size;
}

size_t DxgiFormatFetcher::key_hasher::operator()(format_desc const& k) const
{
    return static_cast<size_t>(k.is_fp) | (static_cast<size_t>(k.is_signed) << 1) | (static_cast<size_t>(k.is_normalized) << 2)
        | (static_cast<size_t>(k.element_count) << 3) | (static_cast<size_t>(k.element_size) << 6);
}


template<typename Variant>
class DxgiTypesMapFiller
{
    constexpr static bool is_fp = FETCH_VALUE(Variant, 0);
    constexpr static bool is_signed = FETCH_VALUE(Variant, 1);
    constexpr static bool is_normalized = FETCH_VALUE(Variant, 2);
    constexpr static unsigned char element_count = FETCH_VALUE(Variant, 3);
    constexpr static unsigned char element_size = FETCH_VALUE(Variant, 4);

public:
    static bool iterate(DxgiFormatFetcher::dxgi_types_map& d3d12_formats)
    {
        constexpr DxgiFormatFetcher::format_desc key{ is_fp, is_signed, is_normalized, element_count, element_size };
        if constexpr (is_fp) {
            if constexpr (element_size == 2) {
                DXGI_FORMAT format = d3d12_type_traits<half, element_count, is_normalized>::dxgi_format;
                d3d12_formats.insert(std::make_pair(key, format));
            }
            else if constexpr (element_size == 4) {
                DXGI_FORMAT format = d3d12_type_traits<float, element_count, is_normalized>::dxgi_format;
                d3d12_formats.insert(std::make_pair(key, format));
            }
        }
        else
        {
            if constexpr (is_signed)
            {
                if constexpr (element_size == 1) {
                    DXGI_FORMAT format = d3d12_type_traits<int8_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats.insert(std::make_pair(key, format));
                }
                else if constexpr (element_size == 2) {
                    DXGI_FORMAT format = d3d12_type_traits<int16_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats.insert(std::make_pair(key, format));
                }
                else if constexpr (element_size == 4) {
                    DXGI_FORMAT format = d3d12_type_traits<int32_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats.insert(std::make_pair(key, format));
                }
            }
            else {
                if constexpr (element_size == 1) {
                    DXGI_FORMAT format = d3d12_type_traits<uint8_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats.insert(std::make_pair(key, format));
                }
                else if constexpr (element_size == 2) {
                    DXGI_FORMAT format = d3d12_type_traits<uint16_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats.insert(std::make_pair(key, format));
                }
                else if constexpr (element_size == 4) {
                    DXGI_FORMAT format = d3d12_type_traits<uint32_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats.insert(std::make_pair(key, format));
                }
            }
        }

        return true;
    }
};


template<typename Variant>
class VertexAttributeSpecificationFactory
{
    using vertex_attribute_format = FETCH_TYPE(Variant, 0);
    constexpr static unsigned char vertex_attribute_size = FETCH_VALUE(Variant, 1);
    constexpr static bool normalized = FETCH_VALUE(Variant, 2);

public:
    static bool iterate(auto& user_data)
    {
        if constexpr (d3d12_type_traits<vertex_attribute_format, vertex_attribute_size, normalized>::dxgi_format == DXGI_FORMAT_UNKNOWN)
        {
            return true;    // unsupported combination of format attributes, continue iterating
        }
        else
        {
            if (user_data.spec.format == StaticTypeToDataFormat<vertex_attribute_format>::data_format
                && user_data.spec.element_count == vertex_attribute_size
                && user_data.spec.is_normalized == normalized)
            {
                user_data.result = std::make_shared<VertexAttributeSpecification<vertex_attribute_format, vertex_attribute_size, normalized>>(
                    user_data.spec.primitive_assembler_input_slot,
                    user_data.spec.element_offset,
                    user_data.spec.name,
                    user_data.spec.name_index,
                    user_data.spec.instancing_data_rate
                );

                return false; // vertex attribute specification has been created, stop iterating
            }
        }

        return true;
    }
};


DxgiFormatFetcher::DxgiFormatFetcher()
{
    using DxgiFormatsIterator = misc::StaticTemplateIterator<
        misc::ValueContainer<bool, false, true>,
        misc::ValueContainer<bool, false, true>,
        misc::ValueContainer<bool, false, true>,
        misc::ValueContainer<unsigned char, 1, 2, 3, 4>,
        misc::ValueContainer<unsigned char, 1, 2, 4>>;

    DxgiFormatsIterator::iterate<DxgiTypesMapFiller>(m_d3d12_formats);

    for (auto const& e : m_d3d12_formats) {
        m_d3d12_format_descriptions.insert(std::make_pair(e.second, e.first));
    }
}

DXGI_FORMAT DxgiFormatFetcher::fetch(bool is_floating_point, bool is_signed, bool is_normalized, unsigned char element_count, unsigned char element_size) const
{
    auto it = m_d3d12_formats.find(format_desc{ .is_fp = is_floating_point, .is_signed = is_signed, .is_normalized = is_normalized,
        .element_count = element_count, .element_size = element_size });
    return it != m_d3d12_formats.end() ? it->second : DXGI_FORMAT_UNKNOWN;
}

DxgiFormatFetcher::format_desc DxgiFormatFetcher::fetch(DXGI_FORMAT format) const
{
    return m_d3d12_format_descriptions.at(format);
}

std::shared_ptr<AbstractVertexAttributeSpecification> DxgiFormatFetcher::createVertexAttribute(va_spec const& spec) const
{
    using VertexAttributeVariantsIterator = misc::StaticTemplateIterator<
        misc::TypeContainer<float, half, uint32_t, int32_t, uint16_t, int16_t, uint8_t, int8_t>,
        misc::ValueContainer<unsigned char, 1, 2, 3, 4>,
        misc::ValueContainer<bool, false, true>>;

    struct _aux
    {
        va_spec const& spec;
        std::shared_ptr<AbstractVertexAttributeSpecification> result;
    }value{ .spec = spec, .result = nullptr };

    VertexAttributeVariantsIterator::iterate<VertexAttributeSpecificationFactory>(value);

    return value.result;
}

}