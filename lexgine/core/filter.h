#ifndef LEXGINE_CORE_FILTER_H

#include <cstdint>
#include <limits>
#include <utility>

#include "depth_stencil_descriptor.h"

namespace lexgine {namespace core {

enum class MinificationFilter
{
    nearest,    //!< nearest point interpolation, no mip-mapping
    linear,    //!< linear interpolation, no mip-mapping
    nearest_mipmap_nearest,    //!< nearest point interpolation within and between mip-levels
    linear_mipmap_nearest,    //!< linear interpolation within mip-level and nearest point interpolation between mip-levels
    nearest_mipmap_linear,    //!< nearest point interpolation within mip-level and linear interpolation between mip-levels
    linear_mipmap_linear,    //!< linear interpolation both within and between mip-levels
    anisotropic,    //!< anisotropic filtering


    // The modes below are named using the following notation minimum_* (maximum_*)
    // These modes fetch the same set of texels that are needed to perform filtration in the corresponding mode * and
    // then return minimum (maximum) of these texels. The texels receiving weights 0 during the filtration fetch are not taken into account

    minimum_nearest,
    minimum_linear,
    minimum_nearest_mipmap_nearest,
    minimum_linear_mipmap_nearest,
    minimum_nearest_mipmap_linear,
    minimum_linear_mipmap_linear,
    minimum_anisotropic,

    maximum_nearest,
    maximum_linear,
    maximum_nearest_mipmap_nearest,
    maximum_linear_mipmap_nearest,
    maximum_nearest_mipmap_linear,
    maximum_linear_mipmap_linear,
    maximum_anisotropic
};

enum class MagnificationFilter
{
    nearest,    //!< nearest point interpolation
    linear,    //!< linear interpolation
    anisotropic,    //!< anisotropic filtering


    // The modes below are named using the following notation minimum_* (maximum_*)
    // These modes fetch the same set of texels that are needed to perform filtration in the corresponding mode * and
    // then return minimum (maximum) of these texels. The texels receiving weights 0 during the filtration fetch are not taken into account

    minimum_nearest,
    minimum_linear,
    minimum_anisotropic,

    maximum_nearest,
    maximum_linear,
    maximum_anisotropic
};


//! Wrapping mode determines how to resolve the boundaries during texture filtering fetches
enum class WrapMode : uint8_t
{
    repeat,    //!< boundaries are resolved periodically
    mirror,    //!< boundaries are resolved anti-periodically
    clamp,    //!< boundaries are clamped
    border    //!< boundaries are set to constant value of the border color
};


//! Defines colors that can be used for boundary resolution in the border mode
enum class BorderColor : uint8_t
{
    transparent_black,
    opaque_black,
    opaque_white
};


//! Encapsulates minification and magnification filters
class FilterPack final
{
public:
    //! Initializes the filter pack. Note that if either @param min_filter or @param mag_filter is not "anisotropy" then @param anisotropy is ignored
    //! if @param comparison is "true" the filter pack is intended for use in comparison mode
    FilterPack(MinificationFilter min_filter, MagnificationFilter mag_filter, uint32_t anisotropy,
        WrapMode wrap_u, WrapMode wrap_v, WrapMode wrap_w,
        BorderColor border_color = BorderColor::opaque_black,
        bool comparison = false, ComparisonFunction comparison_function = ComparisonFunction::always,
        float lod_bias = 0.0f, float min_lod = 0.0f, float max_lod = FLT_MAX);

    MinificationFilter MinFilter() const;    //! returns minification filter encapsulated by the pack
    MagnificationFilter MagFilter() const;    //! returns magnification filter encapsulated by the pack

    uint32_t getAnisotropyLevel() const;    //! returns current anisotropy filter of the pack

    std::pair<WrapMode, WrapMode> getWrapModeUV() const;    //! returns boundary resolution modes along U- and V- texture axes packed into std::pair object in this order
    WrapMode getWrapModeW() const;    //! returns boundary resolution mode along W- texture axis
    BorderColor getBorderColor() const;

    bool isComparison() const;    //! returns "true" if this filter pack has to be used in comparison texture sampling mode
    ComparisonFunction getComparisonFunction() const;

    float getLODBias() const;
    std::pair<float, float> getMinMaxLOD() const;    //! returns minimal and maximal LOD-levels permitted to sample from packed into an std::pair in this order

private:
    MinificationFilter m_min_filter;
    MagnificationFilter m_mag_filter;
    WrapMode m_u_wrapping, m_v_wrapping, m_w_wrapping;
    float m_mip_lod_bias;
    uint32_t m_anisotropy_level;
    bool const m_comparison_mode;    //!< "true" if the pack is intended for use in comparison mode
    ComparisonFunction m_cmp_fun;
    BorderColor m_border_color;
    float m_min_lod, m_max_lod;
};

}}

#define LEXGINE_CORE_FILTER_H
#endif
