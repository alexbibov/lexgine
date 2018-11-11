#include "filter.h"

using namespace lexgine::core;


FilterPack::FilterPack(MinificationFilter min_filter,
    MagnificationFilter mag_filter,
    uint32_t max_anisotropy,
    WrapMode wrap_u,
    WrapMode wrap_v,
    WrapMode wrap_w,
    StaticBorderColor static_border_color,
    bool comparison,
    ComparisonFunction comparison_function,
    float mip_lod_bias,
    float min_lod,
    float max_lod) :
    m_min_filter{ min_filter },
    m_mag_filter{ mag_filter },
    m_u_wrapping{ wrap_u },
    m_v_wrapping{ wrap_v },
    m_w_wrapping{ wrap_w },
    m_mip_lod_bias{ mip_lod_bias },
    m_max_anisotropy_level{ max_anisotropy },
    m_comparison_mode{ comparison },
    m_cmp_fun{ comparison_function },
    m_static_border_color{ static_border_color },
    m_min_lod{ min_lod },
    m_max_lod{ max_lod }
{

}




MinificationFilter FilterPack::MinFilter() const
{
    return m_min_filter;
}

MagnificationFilter FilterPack::MagFilter() const
{
    return m_mag_filter;
}

uint32_t FilterPack::getMaximalAnisotropyLevel() const
{
    return m_max_anisotropy_level;
}

std::pair<WrapMode, WrapMode> FilterPack::getWrapModeUV() const
{
    return std::make_pair(m_u_wrapping, m_v_wrapping);
}

WrapMode FilterPack::getWrapModeW() const
{
    return m_w_wrapping;
}

StaticBorderColor FilterPack::getStaticBorderColor() const
{
    return m_static_border_color;
}

bool FilterPack::isComparison() const
{
    return m_comparison_mode;
}

ComparisonFunction FilterPack::getComparisonFunction() const
{
    return m_cmp_fun;
}

float FilterPack::getMipLODBias() const
{
    return m_mip_lod_bias;
}

std::pair<float, float> lexgine::core::FilterPack::getMinMaxLOD() const
{
    return std::make_pair(m_min_lod, m_max_lod);
}
