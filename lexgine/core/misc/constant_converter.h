#ifndef LEXGINE_CORE_MISC_CONSTANT_CONVERTER_H
#define LEXGINE_CORE_MISC_CONSTANT_CONVERTER_H

#include <d3d12.h>

#include "misc.h"
#include "lexgine/core/blend_descriptor.h"
#include "lexgine/core/rasterizer_descriptor.h"
#include "lexgine/core/depth_stencil_descriptor.h"
#include "lexgine/core/primitive_topology.h"
#include "lexgine/core/filter.h"

namespace lexgine::core::misc {

//! Converts blending state constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineAPI API, BlendFactor blend_factor> struct BlendFactorConverter;

//! Converts blending operation constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineAPI API, BlendOperation blend_op> struct BlendOperationConverter;

//! Converts blending logical operation constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineAPI API, BlendLogicalOperation blend_logical_op> struct BlendLogicalOperationConverter;

//! Converts triangle fill mode constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineAPI API, FillMode triangle_fill_mode> struct FillModeConverter;

//! Converts cull mode constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineAPI API, CullMode cull_mode> struct CullModeConverter;

//! Converts conservative rasterization mode constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineAPI API, ConservativeRasterizationMode cull_mode> struct ConservativeRasterizationModeConverter;

//! Converts comparison condition constants from API agnostic values to the values prescribed by certain graphics APIs
template<EngineAPI API, ComparisonFunction cmp_fun> struct ComparisonFunctionConverter;

//! Converts stencil operation constants from API agnostic values to the values prescribed by certain graphics APIs
template<EngineAPI API, StencilOperation stencil_op> struct StencilOperationConverter;

//! Converts constants describing primitive topology type from API agnostic values to the values prescribed by certain graphics APIs
template<EngineAPI API, PrimitiveTopology primitive_topology> struct PrimitiveTopologyConverter;

//! Converts constants describing minification and magnification filters from API agnostic values to the values prescribed by certain graphics APIs
template<EngineAPI API, MinificationFilter min_filter, MagnificationFilter mag_filter, bool is_comparison = false> struct MinMagFilterConverter;

//! Uses API-agnostic constant describing JUST minification filter and tries to convert it to the requested API constant. This may be not supported by certain APIs
template<EngineAPI API, MinificationFilter min_filter, bool is_comparison = false> struct MinificationFilterConverter;

//! Uses API-agnostic constant describing JUST magnification filter and tries to convert it to the requested API constant. This may be not supported by certain APIs
template<EngineAPI API, MagnificationFilter min_filter, bool is_comparison = false> struct MagnificationFilterConverter;

//! Converts API-agnostic constant determining sampling boundary resolution mode to values accepted by certain graphics APIs
template<EngineAPI API, WrapMode wrap_mode> struct WrapModeConverter;

//! Converts API-agnostic constant determining border color used for sampling boundary resolution to the corresponding API-specific value
template<EngineAPI API, BorderColor border_color> struct BorderColorConverter;

}

#endif
