#include <algorithm>
#include <array>

#include <engine/core/dx/d3d12/d3d12_tools.h>
#include <engine/core/misc/template_argument_iterator.h>

namespace lexgine::core::dx::d3d12 {

bool DxgiFormatFetcher::key::operator==(key const& other) const
{
    return is_fp == other.is_fp && is_signed == other.is_signed
        && is_normalized == other.is_normalized && element_count == other.element_count
        && element_size == other.element_size;
}

size_t DxgiFormatFetcher::key_hasher::operator()(key const& k) const
{
    return static_cast<size_t>(k.is_fp) | (static_cast<size_t>(k.is_signed) << 1) | (static_cast<size_t>(k.is_normalized) << 2)
        | (static_cast<size_t>(k.element_count) << 3) | (static_cast<size_t>(k.element_size) << 6);
}

namespace DxgiTypeTraitsArguments
{
    using is_fp_vals = misc::value_arg_pack<bool, false, true>;
    using is_signed_vals = misc::value_arg_pack<bool, false, true>;
    using is_normalized_vals = misc::value_arg_pack<bool, false, true>;
    using element_count_vals = misc::value_arg_pack<unsigned char, 1, 2, 3, 4>;
    using element_size_vals = misc::value_arg_pack<unsigned char, 1, 2, 4>;
};

template<typename TupleListType = void>
class DxgiTypeTraitsIterator
{
public:
    static bool iterate(void* user_data)
    {
        constexpr bool is_fp = misc::get_tuple_element<TupleListType, 0>::value;
        constexpr bool is_signed = misc::get_tuple_element<TupleListType, 1>::value;
        constexpr bool is_normalized = misc::get_tuple_element<TupleListType, 2>::value;
        constexpr auto element_count = misc::get_tuple_element<TupleListType, 3>::value;
        constexpr auto element_size = misc::get_tuple_element<TupleListType, 4>::value;

        DxgiFormatFetcher::dxgi_types_map& d3d12_formats_map = *reinterpret_cast<DxgiFormatFetcher::dxgi_types_map*>(user_data);
        constexpr DxgiFormatFetcher::key key{ is_fp, is_signed, is_normalized, element_count, element_size };
        if constexpr (is_fp) {
            if constexpr (element_size == 2) {
                DXGI_FORMAT format = d3d12_type_traits<half, element_count, is_normalized>::dxgi_format;
                d3d12_formats_map.insert(std::make_pair(key, format));
            }
            else if constexpr (element_size == 4) {
                DXGI_FORMAT format = d3d12_type_traits<float, element_count, is_normalized>::dxgi_format;
                d3d12_formats_map.insert(std::make_pair(key, format));
            }
        }
        else
        {
            if constexpr (is_signed)
            {
                if constexpr (element_size == 1) {
                    DXGI_FORMAT format = d3d12_type_traits<int8_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats_map.insert(std::make_pair(key, format));
                }
                else if constexpr (element_size == 2) {
                    DXGI_FORMAT format = d3d12_type_traits<int16_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats_map.insert(std::make_pair(key, format));
                }
                else if constexpr (element_size == 4) {
                    DXGI_FORMAT format = d3d12_type_traits<int32_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats_map.insert(std::make_pair(key, format));
                }
            }
            else {
                if constexpr (element_size == 1) {
                    DXGI_FORMAT format = d3d12_type_traits<uint8_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats_map.insert(std::make_pair(key, format));
                }
                else if constexpr (element_size == 2) {
                    DXGI_FORMAT format = d3d12_type_traits<uint16_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats_map.insert(std::make_pair(key, format));
                }
                else if constexpr (element_size == 4) {
                    DXGI_FORMAT format = d3d12_type_traits<uint32_t, element_count, is_normalized>::dxgi_format;
                    d3d12_formats_map.insert(std::make_pair(key, format));
                }
            }
        }

        return true;
    }
};

DxgiFormatFetcher::DxgiFormatFetcher()
{
    misc::TemplateArgumentIterator<DxgiTypeTraitsIterator,
        DxgiTypeTraitsArguments::is_fp_vals,
        DxgiTypeTraitsArguments::is_signed_vals,
        DxgiTypeTraitsArguments::is_normalized_vals,
        DxgiTypeTraitsArguments::element_count_vals,
        DxgiTypeTraitsArguments::element_size_vals>::loop(&m_d3d12_formats);

}

DXGI_FORMAT DxgiFormatFetcher::fetch(bool is_floating_point, bool is_signed, bool is_normalized, unsigned char element_count, unsigned char element_size) const
{
    auto it = m_d3d12_formats.find(key{ .is_fp = is_floating_point, .is_signed = is_signed, .is_normalized = is_normalized,
        .element_count = element_count, .element_size = element_size });
    return it != m_d3d12_formats.end() ? it->second : DXGI_FORMAT_UNKNOWN;
}

}