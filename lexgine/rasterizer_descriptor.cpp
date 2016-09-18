#include "rasterizer_descriptor.h"

using namespace lexgine::core;

RasterizerDescriptor::RasterizerDescriptor(FillMode fill_mode, CullMode cull_mode, FrontFaceWinding winding,
    int depth_bias, float depth_bias_clamp, float slope_scaled_depth_bias, bool depth_clip_enable,
    bool multi_sampling_enable, bool anti_aliased_line_drawing,
    ConservativeRasterizationMode conservative_rasterization_mode) :
    m_fill_mode{ fill_mode },
    m_cull_mode{ cull_mode },
    m_winding_order{ winding },
    m_depth_bias{ depth_bias },
    m_depth_bias_clamp{ depth_bias_clamp },
    m_slope_scaled_depth_bias{ slope_scaled_depth_bias },
    m_depth_clip_enable{ depth_clip_enable },
    m_anti_aliased_line_enable{ anti_aliased_line_drawing },
    m_multi_sampling_enable{ anti_aliased_line_drawing ? false : multi_sampling_enable },
    m_cr_mode{ conservative_rasterization_mode }
{

}

FillMode RasterizerDescriptor::getFillMode() const
{
    return m_fill_mode;
}

CullMode RasterizerDescriptor::getCullMode() const
{
    return m_cull_mode;
}

FrontFaceWinding RasterizerDescriptor::getWindingOrder() const
{
    return m_winding_order;
}

int RasterizerDescriptor::getDepthBias() const
{
    return m_depth_bias;
}

float RasterizerDescriptor::getDepthBiasClamp() const
{
    return m_depth_bias_clamp;
}

float RasterizerDescriptor::getSlopeScaledDepthBias() const
{
    return m_slope_scaled_depth_bias;
}

bool RasterizerDescriptor::isDepthClipEnabled() const
{
    return m_depth_clip_enable;
}

bool RasterizerDescriptor::isMultisamplingEnabled() const
{
    return m_multi_sampling_enable;
}

bool RasterizerDescriptor::isAntialiasedLineDrawingEnabled() const
{
    return m_anti_aliased_line_enable;
}

ConservativeRasterizationMode RasterizerDescriptor::getConservativeRasterizationMode() const
{
    return m_cr_mode;
}
