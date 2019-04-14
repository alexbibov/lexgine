#ifndef LEXGINE_CORE_VULKAN_VULKAN_TOOLS_H


#include "constant_converter.h"


namespace lexgine {namespace core {namespace misc {

template<BlendFactor blend_factor>
struct BlendFactorConverter<EngineAPI::Vulkan, blend_factor>;   // NOTICE_TO_DEVELOPER: to be implemented

template<BlendOperation blend_op>
struct BlendOperationConverter<EngineAPI::Vulkan, blend_op>;   // NOTICE_TO_DEVELOPER: to be implemented

template<BlendLogicalOperation blend_logical_op>
struct BlendLogicalOperationConverter<EngineAPI::Vulkan, blend_logical_op>;   // NOTICE_TO_DEVELOPER: to be implemented

template<FillMode triangle_fill_mode>
struct FillModeConverter<EngineAPI::Vulkan, triangle_fill_mode>;   // NOTICE_TO_DEVELOPER: to be implemented

template<CullMode cull_mode>
struct CullModeConverter<EngineAPI::Vulkan, cull_mode>;   // NOTICE_TO_DEVELOPER: to be implemented

template<ConservativeRasterizationMode conservative_rasterization_mode>
struct ConservativeRasterizationModeConverter<EngineAPI::Vulkan, conservative_rasterization_mode>;   // NOTICE_TO_DEVELOPER: to be implemented

template<ComparisonFunction cmp_fun>
struct ComparisonFunctionConverter<EngineAPI::Vulkan, cmp_fun>;   // NOTICE_TO_DEVELOPER: to be implemented

template<StencilOperation stencil_op>
struct StencilOperationConverter<EngineAPI::Vulkan, stencil_op>;   // NOTICE_TO_DEVELOPER: to be implemented

template<PrimitiveTopologyType primitive_topology>
struct PrimitiveTopologyConverter<EngineAPI::Vulkan, primitive_topology>;    // NOTICE_TO_DEVELOPER: to be implemented

}}}

#define LEXGINE_CORE_VULKAN_VULKAN_TOOLS_H
#endif
