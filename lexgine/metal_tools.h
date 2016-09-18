#ifndef LEXGINE_CORE_MISC_METAL_METAL_TOOLS_H

namespace lexgine {namespace core {namespace misc {

template<BlendFactor blend_factor>
struct BlendFactorConverter<EngineAPI::Metal, blend_factor>;   // NOTICE_TO_DEVELOPER: to be implemented

template<BlendOperation blend_op>
struct BlendOperationConverter<EngineAPI::Metal, blend_op>;   // NOTICE_TO_DEVELOPER: to be implemented

template<BlendLogicalOperation blend_logical_op>
struct BlendLogicalOperationConverter<EngineAPI::Metal, blend_logical_op>;   // NOTICE_TO_DEVELOPER: to be implemented

template<FillMode triangle_fill_mode>
struct FillModeConverter<EngineAPI::Metal, triangle_fill_mode>;   // NOTICE_TO_DEVELOPER: to be implemented

template<CullMode cull_mode>
struct CullModeConverter<EngineAPI::Metal, cull_mode>;   // NOTICE_TO_DEVELOPER: to be implemented

template<ConservativeRasterizationMode conservative_rasterization_mode>
struct ConservativeRasterizationModeConverter<EngineAPI::Metal, conservative_rasterization_mode>;   // NOTICE_TO_DEVELOPER: to be implemented

template<ComparisonFunction cmp_fun>
struct ComparisonFunctionConverter<EngineAPI::Metal, cmp_fun>;   // NOTICE_TO_DEVELOPER: to be implemented

template<StencilOperation stencil_op>
struct StencilOperationConverter<EngineAPI::Metal, stencil_op>;   // NOTICE_TO_DEVELOPER: to be implemented

template<PrimitiveTopology primitive_topology>
struct PrimitiveTopologyConverter<EngineAPI::Metal, primitive_topology>;    // NOTICE_TO_DEVELOPER: to be implemented

}}}

#define LEXGINE_CORE_MISC_METAL_METAL_TOOLS_H
#endif
