#ifndef LEXGINE_CORE_MISC_DX_D3D12_D3D12_TOOLS_H

#include "constant_converter.h"

namespace lexgine {namespace core {namespace misc {

template<BlendFactor blend_factor>
struct BlendFactorConverter<EngineAPI::Direct3D12, blend_factor>
{
    // See definition of BlendFactor and of D3D12_BLEND for grim details
    static uint8_t constexpr value()
    {
        return static_cast<uint8_t>(blend_factor) <= 10 ?
            static_cast<uint8_t>(blend_factor) + 1 : static_cast<uint8_t>(blend_factor) + 3;
    }
};

//! Helper: converts API agnostic blend factor constant defined at runtime to Direct3D 12 specific value
inline D3D12_BLEND d3d12Convert(BlendFactor blend_factor)
{
    switch (blend_factor)
    {
    case BlendFactor::zero:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::zero>::value());
    case BlendFactor::one:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::one>::value());
    case BlendFactor::source_color:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::source_color>::value());
    case BlendFactor::one_minus_source_color:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::one_minus_source_color>::value());
    case BlendFactor::source_alpha:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::source_alpha>::value());
    case BlendFactor::one_minus_source_alpha:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::one_minus_source_alpha>::value());
    case BlendFactor::destination_alpha:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::destination_alpha>::value());
    case BlendFactor::one_minus_destination_alpha:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::one_minus_destination_alpha>::value());
    case BlendFactor::destination_color:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::destination_color>::value());
    case BlendFactor::one_minus_destination_color:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::one_minus_destination_color>::value());
    case BlendFactor::source_alpha_saturation:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::source_alpha_saturation>::value());
    case BlendFactor::custom_constant:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::custom_constant>::value());
    case BlendFactor::one_minus_custom_constant:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::one_minus_custom_constant>::value());
    case BlendFactor::source1_color:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::source1_color>::value());
    case BlendFactor::one_minus_source1_color:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::one_minus_source1_color>::value());
    case BlendFactor::source1_alpha:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::source1_alpha>::value());
    case BlendFactor::one_minus_source1_alpha:
        return static_cast<D3D12_BLEND>(BlendFactorConverter<EngineAPI::Direct3D12, BlendFactor::one_minus_source1_alpha>::value());
    default: throw;    // not supported
    }
}


template<BlendOperation blend_op>
struct BlendOperationConverter<EngineAPI::Direct3D12, blend_op>
{
    // See definition of BlendOperation and of D3D12_BLEND_OP for dark details
    static uint8_t constexpr value()
    {
        return static_cast<uint8_t>(blend_op) + 1;
    }
};

//! Helper: converts API agnostic blend operation constant to the corresponding enumeration value specific for Direct3D 12
inline D3D12_BLEND_OP d3d12Convert(BlendOperation blend_op)
{
    switch (blend_op)
    {
    case BlendOperation::add:
        return static_cast<D3D12_BLEND_OP>(BlendOperationConverter<EngineAPI::Direct3D12, BlendOperation::add>::value());
    case BlendOperation::subtract:
        return static_cast<D3D12_BLEND_OP>(BlendOperationConverter<EngineAPI::Direct3D12, BlendOperation::subtract>::value());
    case BlendOperation::reverse_subtract:
        return static_cast<D3D12_BLEND_OP>(BlendOperationConverter<EngineAPI::Direct3D12, BlendOperation::reverse_subtract>::value());
    case BlendOperation::min:
        return static_cast<D3D12_BLEND_OP>(BlendOperationConverter<EngineAPI::Direct3D12, BlendOperation::min>::value());
    case BlendOperation::max:
        return static_cast<D3D12_BLEND_OP>(BlendOperationConverter<EngineAPI::Direct3D12, BlendOperation::max>::value());
    default: throw;    // not supported
    }
}


template<BlendLogicalOperation blend_logical_op>
struct BlendLogicalOperationConverter<EngineAPI::Direct3D12, blend_logical_op>
{
    // See definition of BlendLogicalOperation and of D3D12_LOGIC_OP for bleak details
    static uint8_t constexpr value()
    {
        return static_cast<uint8_t>(blend_logical_op);
    }
};

//! Converts API-agnostic blend logical operation to the corresponding Direc3D 12 enumeration
inline D3D12_LOGIC_OP d3d12Convert(BlendLogicalOperation blend_logical_op)
{
    switch (blend_logical_op)
    {
    case BlendLogicalOperation::clear:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::clear>::value());
    case BlendLogicalOperation::set:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::set>::value());
    case BlendLogicalOperation::copy:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::copy>::value());
    case BlendLogicalOperation::copy_inverted:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::copy_inverted>::value());
    case BlendLogicalOperation::no_operation:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::no_operation>::value());
    case BlendLogicalOperation::invert:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::invert>::value());
    case BlendLogicalOperation::and:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::and>::value());
    case BlendLogicalOperation::nand:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::nand>::value());
    case BlendLogicalOperation:: or :
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::or>::value());
    case BlendLogicalOperation::nor:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::nor>::value());
    case BlendLogicalOperation::xor:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::xor>::value());
    case BlendLogicalOperation::equiv:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::equiv>::value());
    case BlendLogicalOperation::and_then_reverse:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::and_then_reverse>::value());
    case BlendLogicalOperation::and_then_invert:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::and_then_invert>::value());
    case BlendLogicalOperation::or_then_reverse:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::or_then_reverse>::value());
    case BlendLogicalOperation::or_then_invert:
        return static_cast<D3D12_LOGIC_OP>(BlendLogicalOperationConverter<EngineAPI::Direct3D12, BlendLogicalOperation::or_then_invert>::value());
    default: throw;    // not supported
    }
}


template<FillMode triangle_fill_mode>
struct FillModeConverter<EngineAPI::Direct3D12, triangle_fill_mode>
{
    // See definition of FillMode and of D3D12_FILL_MODE for dirty details
    static uint8_t constexpr value()
    {
        return static_cast<uint8_t>(triangle_fill_mode) + 2;
    }

};

//! Converts API-agnostic constant determining triangle fill mode to the corresponding value, which is specific to Direct3D 12
inline D3D12_FILL_MODE d3d12Convert(FillMode fill_mode)
{
    switch (fill_mode)
    {
    case FillMode::wireframe:
        return static_cast<D3D12_FILL_MODE>(FillModeConverter<EngineAPI::Direct3D12, FillMode::wireframe>::value());
    case FillMode::solid:
        return static_cast<D3D12_FILL_MODE>(FillModeConverter<EngineAPI::Direct3D12, FillMode::solid>::value());
    default: throw;    // not supported
    }
}

template<CullMode cull_mode>
struct CullModeConverter<EngineAPI::Direct3D12, cull_mode>
{
    // See definition of CullMode and of D3D12_CULL_MODE for more details
    static uint8_t constexpr value()
    {
        return static_cast<uint8_t>(cull_mode) + 1;
    }
};

//! Converts API-agnostic cull mode constant to the coresponding Direct3D 12 enumeration value
inline D3D12_CULL_MODE d3d12Convert(CullMode cull_mode)
{
    switch (cull_mode)
    {
    case CullMode::none:
        return static_cast<D3D12_CULL_MODE>(CullModeConverter<EngineAPI::Direct3D12, CullMode::none>::value());
    case CullMode::front:
        return static_cast<D3D12_CULL_MODE>(CullModeConverter<EngineAPI::Direct3D12, CullMode::front>::value());
    case CullMode::back:
        return static_cast<D3D12_CULL_MODE>(CullModeConverter<EngineAPI::Direct3D12, CullMode::back>::value());
    default: throw;    //not supported
    }
}

template<ConservativeRasterizationMode conservative_rasterization_mode>
struct ConservativeRasterizationModeConverter<EngineAPI::Direct3D12, conservative_rasterization_mode>
{
    // See definition of ConservativeRasterizationMode and of D3D12_CONSERVATIVE_RASTERIZATION_MODE for enlightening details
    static uint8_t constexpr value()
    {
        return static_cast<uint8_t>(conservative_rasterization_mode);
    }
};

//! Converts API-agnostic constant determining conservative rasterization mode to the corresponding Direct3D 12 specific value
inline D3D12_CONSERVATIVE_RASTERIZATION_MODE d3d12Convert(ConservativeRasterizationMode conservative_rasterization_mode)
{
    switch (conservative_rasterization_mode)
    {
    case ConservativeRasterizationMode::off:
        return static_cast<D3D12_CONSERVATIVE_RASTERIZATION_MODE>(ConservativeRasterizationModeConverter<EngineAPI::Direct3D12, ConservativeRasterizationMode::off>::value());
    case ConservativeRasterizationMode::on:
        return static_cast<D3D12_CONSERVATIVE_RASTERIZATION_MODE>(ConservativeRasterizationModeConverter<EngineAPI::Direct3D12, ConservativeRasterizationMode::on>::value());
    default: throw;    // not supported
    }
}

template<ComparisonFunction cmp_fun>
struct ComparisonFunctionConverter<EngineAPI::Direct3D12, cmp_fun>
{
    // See definition of ComparisonFunction and of D3D12_COMPARISON_FUNC for non-confusing details
    static uint8_t constexpr value()
    {
        return static_cast<uint8_t>(cmp_fun) + 1;
    }
};


//! Converts API-agnostic comparison function constant to Direct3D 12 specific value
//! This function allows to use runtime values on the contrast to the coupled template, which can naturally act over only compile time constants
inline D3D12_COMPARISON_FUNC d3d12Convert(ComparisonFunction cmp_fun)
{
    switch (cmp_fun)
    {
    case ComparisonFunction::never:
        return static_cast<D3D12_COMPARISON_FUNC>(ComparisonFunctionConverter<EngineAPI::Direct3D12, ComparisonFunction::never>::value());
    case ComparisonFunction::less:
        return static_cast<D3D12_COMPARISON_FUNC>(ComparisonFunctionConverter<EngineAPI::Direct3D12, ComparisonFunction::less>::value());
    case ComparisonFunction::equal:
        return static_cast<D3D12_COMPARISON_FUNC>(ComparisonFunctionConverter<EngineAPI::Direct3D12, ComparisonFunction::equal>::value());
    case ComparisonFunction::less_or_equal:
        return static_cast<D3D12_COMPARISON_FUNC>(ComparisonFunctionConverter<EngineAPI::Direct3D12, ComparisonFunction::less_or_equal>::value());
    case ComparisonFunction::greater:
        return static_cast<D3D12_COMPARISON_FUNC>(ComparisonFunctionConverter<EngineAPI::Direct3D12, ComparisonFunction::greater>::value());
    case ComparisonFunction::not_equal:
        return static_cast<D3D12_COMPARISON_FUNC>(ComparisonFunctionConverter<EngineAPI::Direct3D12, ComparisonFunction::not_equal>::value());
    case ComparisonFunction::greater_equal:
        return static_cast<D3D12_COMPARISON_FUNC>(ComparisonFunctionConverter<EngineAPI::Direct3D12, ComparisonFunction::greater_equal>::value());
    case ComparisonFunction::always:
        return static_cast<D3D12_COMPARISON_FUNC>(ComparisonFunctionConverter<EngineAPI::Direct3D12, ComparisonFunction::always>::value());
    default: throw;    // not supported
    }
}



template<StencilOperation stencil_op>
struct StencilOperationConverter<EngineAPI::Direct3D12, stencil_op>
{
    // See definition of StencilOperation and of D3D12_STENCIL_OP for the devil-living-in-the-details
    static uint8_t constexpr value()
    {
        return static_cast<uint8_t>(stencil_op) + 1;
    }
};

//! Converts API-agnostic constant determining stencil buffer update operation to the corresponding Direct3D 12 enumeration value
inline D3D12_STENCIL_OP d3d12Convert(StencilOperation stencil_op)
{
    switch (stencil_op)
    {
    case StencilOperation::keep:
        return static_cast<D3D12_STENCIL_OP>(StencilOperationConverter<EngineAPI::Direct3D12, StencilOperation::keep>::value());
    case StencilOperation::zero:
        return static_cast<D3D12_STENCIL_OP>(StencilOperationConverter<EngineAPI::Direct3D12, StencilOperation::zero>::value());
    case StencilOperation::replace:
        return static_cast<D3D12_STENCIL_OP>(StencilOperationConverter<EngineAPI::Direct3D12, StencilOperation::replace>::value());
    case StencilOperation::increment_and_saturate:
        return static_cast<D3D12_STENCIL_OP>(StencilOperationConverter<EngineAPI::Direct3D12, StencilOperation::increment_and_saturate>::value());
    case StencilOperation::decrement_and_saturate:
        return static_cast<D3D12_STENCIL_OP>(StencilOperationConverter<EngineAPI::Direct3D12, StencilOperation::decrement_and_saturate>::value());
    case StencilOperation::invert:
        return static_cast<D3D12_STENCIL_OP>(StencilOperationConverter<EngineAPI::Direct3D12, StencilOperation::invert>::value());
    case StencilOperation::increment:
        return static_cast<D3D12_STENCIL_OP>(StencilOperationConverter<EngineAPI::Direct3D12, StencilOperation::increment>::value());
    case StencilOperation::decrement:
        return static_cast<D3D12_STENCIL_OP>(StencilOperationConverter<EngineAPI::Direct3D12, StencilOperation::decrement>::value());
    default: throw;    // not supported
    }
}


template<PrimitiveTopology primitive_topology>
struct PrimitiveTopologyConverter<EngineAPI::Direct3D12, primitive_topology>
{
    // See definition of PrimitiveTopology and of D3D12_PRIMITIVE_TOPOLOGY_TYPE for the clumsy details
    static uint8_t constexpr value()
    {
        return static_cast<uint8_t>(primitive_topology) + 1;
    }
};

//! Converts primitive topology constant from API-agnostic value to the value specific to Direct3D 12
inline D3D12_PRIMITIVE_TOPOLOGY_TYPE d3d12Convert(PrimitiveTopology primitive_topology)
{
    switch (primitive_topology)
    {
    case PrimitiveTopology::point:
        return static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(PrimitiveTopologyConverter<EngineAPI::Direct3D12, PrimitiveTopology::point>::value());
    case PrimitiveTopology::line:
        return static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(PrimitiveTopologyConverter<EngineAPI::Direct3D12, PrimitiveTopology::line>::value());
    case PrimitiveTopology::triangle:
        return static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(PrimitiveTopologyConverter<EngineAPI::Direct3D12, PrimitiveTopology::triangle>::value());
    case PrimitiveTopology::patch:
        return static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(PrimitiveTopologyConverter<EngineAPI::Direct3D12, PrimitiveTopology::patch>::value());
    default: throw;    // not supported
    }
}


#include "d3d12_min_mag_filter_converter.inl"


template<WrapMode wrap_mode>
struct WrapModeConverter<EngineAPI::Direct3D12, wrap_mode>
{
    // See definition of WrapMode and of D3D12_TEXTURE_ADDRESS_MODE for not-so-easy-to-understand details
    static uint8_t constexpr value()
    {
        return static_cast<uint8_t>(wrap_mode) + 1;
    }
};

//! Converts runtime sampling boundary resolution mode constant to the enumeration value required by Direct3D 12 specs
inline D3D12_TEXTURE_ADDRESS_MODE d3d12Convert(WrapMode wrap_mode)
{
    switch (wrap_mode)
    {
    case WrapMode::repeat:
        return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(WrapModeConverter<EngineAPI::Direct3D12, WrapMode::repeat>::value());
    case WrapMode::mirror:
        return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(WrapModeConverter<EngineAPI::Direct3D12, WrapMode::mirror>::value());
    case WrapMode::clamp:
        return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(WrapModeConverter<EngineAPI::Direct3D12, WrapMode::clamp>::value());
    case WrapMode::border:
        return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(WrapModeConverter<EngineAPI::Direct3D12, WrapMode::border>::value());
    default: throw;    // unsupported
    }
}


template<BorderColor border_color>
struct BorderColorConverter<EngineAPI::Direct3D12, border_color>
{
    // See definition of BorderColor and D3D12_STATIC_BORDER_COLOR enumeration for very detailed details
    static uint8_t constexpr value() { return static_cast<uint8_t>(border_color); }
};

//! Converts runtime value of the border color to Direct3D 12 specific constant
inline D3D12_STATIC_BORDER_COLOR d3d12Convert(BorderColor border_color)
{
    switch (border_color)
    {
    case BorderColor::transparent_black:
        return static_cast<D3D12_STATIC_BORDER_COLOR>(BorderColorConverter<EngineAPI::Direct3D12, BorderColor::transparent_black>::value());
    case BorderColor::opaque_black:
        return static_cast<D3D12_STATIC_BORDER_COLOR>(BorderColorConverter<EngineAPI::Direct3D12, BorderColor::opaque_black>::value());
    case BorderColor::opaque_white:
        return static_cast<D3D12_STATIC_BORDER_COLOR>(BorderColorConverter<EngineAPI::Direct3D12, BorderColor::opaque_white>::value());
    default: throw;    // not supported
    }
}


#include "d3d12_type_traits.inl"

}}}



// HLSL vector types

#include "vector_types.h"
namespace lexgine {namespace core {namespace math {

typedef vector4i int4;
typedef vector4u uint4;
typedef vector4f float4;
typedef vector4d double4;
typedef vector4b bool4;

typedef vector3i int3;
typedef vector3u uint3;
typedef vector3f float3;
typedef vector3d double3;
typedef vector3b bool3;

typedef vector2i int2;
typedef vector2u uint2;
typedef vector2f float2;
typedef vector2d double2;
typedef vector2b bool2;

} } }



// HlSL matrix types
#include "matrix_types.h"
namespace lexgine {namespace core {namespace math {

typedef shader_matrix_type<float, 4, 4> float4x4;
typedef shader_matrix_type<float, 3, 3> float3x3;
typedef shader_matrix_type<float, 2, 2> float2x2;
typedef shader_matrix_type<float, 4, 3> float4x3;
typedef shader_matrix_type<float, 4, 2> float4x2;
typedef shader_matrix_type<float, 3, 4> float3x4;
typedef shader_matrix_type<float, 2, 4> float2x4;
typedef shader_matrix_type<float, 3, 2> float3x2;
typedef shader_matrix_type<float, 2, 3> float2x3;

typedef shader_matrix_type<double, 4, 4> double4x4;
typedef shader_matrix_type<double, 3, 3> double3x3;
typedef shader_matrix_type<double, 2, 2> double2x2;
typedef shader_matrix_type<double, 4, 3> double4x3;
typedef shader_matrix_type<double, 4, 2> double4x2;
typedef shader_matrix_type<double, 3, 4> double3x4;
typedef shader_matrix_type<double, 2, 4> double2x4;
typedef shader_matrix_type<double, 3, 2> double3x2;
typedef shader_matrix_type<double, 2, 3> double2x3;

}}}



#define LEXGINE_CORE_MISC_DX_D3D12_D3D12_TOOLS_H
#endif
