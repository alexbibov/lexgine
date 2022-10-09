#ifndef LEXGINE_CORE_MISC_CONSTANT_CONVERTER_H
#define LEXGINE_CORE_MISC_CONSTANT_CONVERTER_H

#include <d3d12.h>

#include "engine/core/engine_api.h"
#include "misc.h"
#include "engine/core/blend_descriptor.h"
#include "engine/core/rasterizer_descriptor.h"
#include "engine/core/depth_stencil_descriptor.h"
#include "engine/core/primitive_topology.h"
#include "engine/core/filter.h"
#include "engine/core/swap_chain_desc.h"

namespace lexgine::core::misc {

//! Converts blending state constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineApi api, BlendFactor blend_factor> struct BlendFactorConverter;

//! Converts blending operation constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineApi api, BlendOperation blend_op> struct BlendOperationConverter;

//! Converts blending logical operation constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineApi api, BlendLogicalOperation blend_logical_op> struct BlendLogicalOperationConverter;

//! Converts triangle fill mode constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineApi api, FillMode triangle_fill_mode> struct FillModeConverter;

//! Converts cull mode constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineApi api, CullMode cull_mode> struct CullModeConverter;

//! Converts conservative rasterization mode constant from API agnostic values to the values prescribed by certain graphics APIs
template<EngineApi api, ConservativeRasterizationMode cull_mode> struct ConservativeRasterizationModeConverter;

//! Converts comparison condition constants from API agnostic values to the values prescribed by certain graphics APIs
template<EngineApi api, ComparisonFunction cmp_fun> struct ComparisonFunctionConverter;

//! Converts stencil operation constants from API agnostic values to the values prescribed by certain graphics APIs
template<EngineApi api, StencilOperation stencil_op> struct StencilOperationConverter;

//! Converts constants describing primitive topology type from API agnostic values to the values prescribed by certain graphics APIs
template<EngineApi api, PrimitiveTopologyType primitive_topology_type> struct PrimitiveTopologyTypeConverter;

//! Converts constants describing primitive topology from API agnostic values to the values understood by requested graphics API
template<EngineApi api, PrimitiveTopology primitive_topology> struct PrimitiveTopologyConverter;

//! Converts constants describing minification and magnification filters from API agnostic values to the values prescribed by certain graphics APIs
template<EngineApi api, MinificationFilter min_filter, MagnificationFilter mag_filter, bool is_comparison = false> struct MinMagFilterConverter;

//! Uses API-agnostic constant describing JUST minification filter and tries to convert it to the requested API constant. This may be not supported by certain APIs
template<EngineApi api, MinificationFilter min_filter, bool is_comparison = false> struct MinificationFilterConverter;

//! Uses API-agnostic constant describing JUST magnification filter and tries to convert it to the requested API constant. This may be not supported by certain APIs
template<EngineApi api, MagnificationFilter min_filter, bool is_comparison = false> struct MagnificationFilterConverter;

//! Converts API-agnostic constant determining sampling boundary resolution mode to values accepted by certain graphics APIs
template<EngineApi api, WrapMode wrap_mode> struct WrapModeConverter;

//! Converts API-agnostic constant determining border color used for sampling boundary resolution to the corresponding API-specific value
template<EngineApi api, StaticBorderColor border_color> struct BorderColorConverter;

//! Converts API-agnostic definition of swap chain color format to an API-specific constant
template<EngineApi api, SwapChainColorFormat> struct SwapChainColorFormatConverter;

//! Converts API-agnostic definition of swap chain depth format to an API-specific constant
template<EngineApi api, SwapChainDepthFormat, bool> struct SwapChainDepthFormatConverter;

}

#endif
