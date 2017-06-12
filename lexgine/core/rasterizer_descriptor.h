#ifndef LEXGINE_CORE_RASTERIZER_DESCRIPTOR

#include <cstdint>

namespace lexgine {namespace core {

//! API-agnostic triangle fill mode
enum class FillMode: int8_t
{
    wireframe, solid
};

//! API-agnostic cull mode
enum class CullMode: int8_t
{
    front, back, none
};

//! API-agnostic conservative rasterization mode
enum class ConservativeRasterizationMode: int8_t
{
    off, on
};

//! API-agnostic winding order assumed for front-facing triangles
enum class FrontFaceWinding
{
    clockwise, counterclockwise
};

//! Encapsulates rasterizer descriptor that contains settings affecting rasterization stage
//! This class is API and OS agnostic. The rasterizer descriptors are intentionally immutable
class RasterizerDescriptor final
{
public:
    RasterizerDescriptor(FillMode fill_mode = FillMode::solid, CullMode cull_mode = CullMode::back, FrontFaceWinding winding = FrontFaceWinding::counterclockwise,
        int depth_bias = 0, float depth_bias_clamp = 0.0f, float slope_scaled_depth_bias = 0.0f, bool depth_clip_enable = true,
        bool multi_sampling_enable = false, bool anti_aliased_line_drawing = false,
        ConservativeRasterizationMode conservative_rasterization_mode = ConservativeRasterizationMode::off);

    FillMode getFillMode() const;
    CullMode getCullMode() const;
    FrontFaceWinding getWindingOrder() const;
    int getDepthBias() const;
    float getDepthBiasClamp() const;
    float getSlopeScaledDepthBias() const;
    bool isDepthClipEnabled() const;
    bool isMultisamplingEnabled() const;
    bool isAntialiasedLineDrawingEnabled() const;
    ConservativeRasterizationMode getConservativeRasterizationMode() const;

private:
    FillMode m_fill_mode;    //!< triangle fill mode
    CullMode m_cull_mode;   //!< triangle cull mode
    FrontFaceWinding m_winding_order;    //!< winding order assumed by front-facing triangles
    int m_depth_bias;    //!< depth value added to a given pixel
    float m_depth_bias_clamp;    //!< maximum depth bias of a pixel
    float m_slope_scaled_depth_bias;    //!< scalar on a given pixel's slope
    bool m_depth_clip_enable;    //!< 'true' if z-clipping is enabled
    bool m_anti_aliased_line_enable;    //! 'true' enables anti-aliased line drawing. Only applies when rendering lines and m_multi_sampling_enable is 'false'
    bool m_multi_sampling_enable;    //!< specifies whether to use quadrilateral (when 'true') or alpha line anti-aliasing (when 'false') algorithm on MSAA render targets
    ConservativeRasterizationMode m_cr_mode;    //! conservative rasterization mode
};

}}

#define LEXGINE_CORE_RASTERIZER_DESCRIPTOR
#endif