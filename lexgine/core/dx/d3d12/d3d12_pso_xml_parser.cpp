#include <algorithm>

#include "d3d12_pso_xml_parser.h"

#include "pugixml.hpp"


using namespace lexgine::core::dx::d3d12;


namespace {

enum class attribute_type {
    boolean,
    primitive_topology,
    unsigned_numeric,
    blend_factor,
    blend_operation,
    blend_logical_operation,
    fill_mode,
    face,
    winding_mode,
    comparison_function,
    stencil_operation,
    depth_stencil_format,
    render_target_format
};

bool isNullAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "NULL") == 0;
}

bool isBooleanAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "TRUE") == 0
        || std::strcmp(attribute.value(), "FALSE") == 0;
}

bool getBooleanFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "TRUE") == 0) return true;
    if (std::strcmp(attribute.value(), "FALSE") == 0) return false;

    return false;
}

bool isPrimitiveTopologyAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "POINT") == 0
        || std::strcmp(attribute.value(), "LINE") == 0
        || std::strcmp(attribute.value(), "TRIANGLE") == 0
        || std::strcmp(attribute.value(), "PATCH") == 0;
}

lexgine::core::PrimitiveTopology getPrimitiveTopologyFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "POINT") == 0) return lexgine::core::PrimitiveTopology::point;
    if (std::strcmp(attribute.value(), "LINE") == 0) return lexgine::core::PrimitiveTopology::line;
    if (std::strcmp(attribute.value(), "TRIANGLE") == 0) return lexgine::core::PrimitiveTopology::triangle;
    if (std::strcmp(attribute.value(), "PATCH") == 0) return lexgine::core::PrimitiveTopology::patch;

    return lexgine::core::PrimitiveTopology::triangle;
}

bool isUnsignedNumericAttribute(pugi::xml_attribute& attribute)
{
    return attribute.as_ullong(0xFFFFFFFFFFFFFFFF) < 0xFFFFFFFFFFFFFFFF;
}

uint32_t getUnsignedNumericFromAttribute(pugi::xml_attribute& attribute)
{
    return attribute.as_uint();
}

bool isBlendFactorAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "ZERO") == 0
        || std::strcmp(attribute.value(), "ONE") == 0
        || std::strcmp(attribute.value(), "SOURCE_COLOR") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_SOURCE_COLOR") == 0
        || std::strcmp(attribute.value(), "SOURCE_ALPHA") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_SOURCE_ALPHA") == 0
        || std::strcmp(attribute.value(), "DESTINATION_ALPHA") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_DESTINATION_ALPHA") == 0
        || std::strcmp(attribute.value(), "DESTINATION_COLOR") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_DESTINATION_COLOR") == 0
        || std::strcmp(attribute.value(), "SOURCE_ALPHA_SATURATION") == 0
        || std::strcmp(attribute.value(), "CUSTOM_CONSTANT") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_CUSTOM_CONSTANT") == 0
        || std::strcmp(attribute.value(), "SOURCE1_COLOR") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_SOURCE1_COLOR") == 0
        || std::strcmp(attribute.value(), "SOURCE1_ALPHA") == 0
        || std::strcmp(attribute.value(), "ONE_MINUS_SOURCE1_ALPHA") == 0;
}

lexgine::core::BlendFactor getBlendFactorFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "ZERO") == 0) return lexgine::core::BlendFactor::zero;
    if (std::strcmp(attribute.value(), "ONE") == 0) return lexgine::core::BlendFactor::one;
    if (std::strcmp(attribute.value(), "SOURCE_COLOR") == 0) return lexgine::core::BlendFactor::source_color;
    if (std::strcmp(attribute.value(), "ONE_MINUS_SOURCE_COLOR") == 0) return lexgine::core::BlendFactor::one_minus_source_color;
    if (std::strcmp(attribute.value(), "SOURCE_ALPHA") == 0) return lexgine::core::BlendFactor::source_alpha;
    if (std::strcmp(attribute.value(), "ONE_MINUS_SOURCE_ALPHA") == 0) return lexgine::core::BlendFactor::one_minus_source_alpha;
    if (std::strcmp(attribute.value(), "DESTINATION_ALPHA") == 0) return lexgine::core::BlendFactor::destination_alpha;
    if (std::strcmp(attribute.value(), "ONE_MINUS_DESTINATION_ALPHA") == 0) return lexgine::core::BlendFactor::one_minus_destination_alpha;
    if (std::strcmp(attribute.value(), "DESTINATION_COLOR") == 0) return lexgine::core::BlendFactor::destination_color;
    if (std::strcmp(attribute.value(), "ONE_MINUS_DESTINATION_COLOR") == 0) return lexgine::core::BlendFactor::one_minus_destination_color;
    if (std::strcmp(attribute.value(), "SOURCE_ALPHA_SATURATION") == 0) return lexgine::core::BlendFactor::source_alpha_saturation;
    if (std::strcmp(attribute.value(), "CUSTOM_CONSTANT") == 0)return lexgine::core::BlendFactor::custom_constant;
    if (std::strcmp(attribute.value(), "ONE_MINUS_CUSTOM_CONSTANT") == 0) return lexgine::core::BlendFactor::one_minus_custom_constant;
    if (std::strcmp(attribute.value(), "SOURCE1_COLOR") == 0)return lexgine::core::BlendFactor::source1_color;
    if (std::strcmp(attribute.value(), "ONE_MINUS_SOURCE1_COLOR") == 0) return lexgine::core::BlendFactor::one_minus_source1_color;
    if (std::strcmp(attribute.value(), "SOURCE1_ALPHA") == 0)return lexgine::core::BlendFactor::source1_alpha;
    if (std::strcmp(attribute.value(), "ONE_MINUS_SOURCE1_ALPHA") == 0) return lexgine::core::BlendFactor::one_minus_source1_alpha;

    return lexgine::core::BlendFactor::zero;
}

bool isBlendOperationAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "ADD") == 0
        || std::strcmp(attribute.value(), "SUBTRACT") == 0
        || std::strcmp(attribute.value(), "REVERSE_SUBTRACT") == 0
        || std::strcmp(attribute.value(), "MIN") == 0
        || std::strcmp(attribute.value(), "MAX") == 0;
}

lexgine::core::BlendOperation getBlendOperationFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "ADD") == 0) return lexgine::core::BlendOperation::add;
    if (std::strcmp(attribute.value(), "SUBTRACT") == 0) return lexgine::core::BlendOperation::subtract;
    if (std::strcmp(attribute.value(), "REVERSE_SUBTRACT") == 0) return lexgine::core::BlendOperation::reverse_subtract;
    if (std::strcmp(attribute.value(), "MIN") == 0) return lexgine::core::BlendOperation::min;
    if (std::strcmp(attribute.value(), "MAX") == 0) return lexgine::core::BlendOperation::max;

    return lexgine::core::BlendOperation::add;
}

bool isBlendLogicalOperationAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "CLEAR") == 0
        || std::strcmp(attribute.value(), "SET") == 0
        || std::strcmp(attribute.value(), "COPY") == 0
        || std::strcmp(attribute.value(), "COPY_INVERTED") == 0
        || std::strcmp(attribute.value(), "NO_OPEATION") == 0
        || std::strcmp(attribute.value(), "INVERT") == 0
        || std::strcmp(attribute.value(), "AND") == 0
        || std::strcmp(attribute.value(), "NAND") == 0
        || std::strcmp(attribute.value(), "OR") == 0
        || std::strcmp(attribute.value(), "NOR") == 0
        || std::strcmp(attribute.value(), "XOR") == 0
        || std::strcmp(attribute.value(), "EQUIV") == 0
        || std::strcmp(attribute.value(), "AND_THEN_REVERSE") == 0
        || std::strcmp(attribute.value(), "AND_THEN_INVERT") == 0
        || std::strcmp(attribute.value(), "OR_THEN_REVERSE") == 0
        || std::strcmp(attribute.value(), "OR_THEN_INVERT") == 0;
}

lexgine::core::BlendLogicalOperation getBlendLogicalOperationFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "CLEAR") == 0) return lexgine::core::BlendLogicalOperation::clear;
    if (std::strcmp(attribute.value(), "SET") == 0) return lexgine::core::BlendLogicalOperation::set;
    if (std::strcmp(attribute.value(), "COPY") == 0) return lexgine::core::BlendLogicalOperation::copy;
    if (std::strcmp(attribute.value(), "COPY_INVERTED") == 0) return lexgine::core::BlendLogicalOperation::copy_inverted;
    if (std::strcmp(attribute.value(), "NO_OPEATION") == 0) return lexgine::core::BlendLogicalOperation::no_operation;
    if (std::strcmp(attribute.value(), "INVERT") == 0) return lexgine::core::BlendLogicalOperation::invert;
    if (std::strcmp(attribute.value(), "AND") == 0) return lexgine::core::BlendLogicalOperation::and;
    if (std::strcmp(attribute.value(), "NAND") == 0) return lexgine::core::BlendLogicalOperation::nand;
    if (std::strcmp(attribute.value(), "OR") == 0) return lexgine::core::BlendLogicalOperation::or;
    if (std::strcmp(attribute.value(), "NOR") == 0) return lexgine::core::BlendLogicalOperation::nor;
    if (std::strcmp(attribute.value(), "XOR") == 0) return lexgine::core::BlendLogicalOperation::xor;
    if (std::strcmp(attribute.value(), "EQUIV") == 0) return lexgine::core::BlendLogicalOperation::equiv;
    if (std::strcmp(attribute.value(), "AND_THEN_REVERSE") == 0) return lexgine::core::BlendLogicalOperation::and_then_reverse;
    if (std::strcmp(attribute.value(), "AND_THEN_INVERT") == 0) return lexgine::core::BlendLogicalOperation::and_then_invert;
    if (std::strcmp(attribute.value(), "OR_THEN_REVERSE") == 0) return lexgine::core::BlendLogicalOperation::or_then_reverse;
    if (std::strcmp(attribute.value(), "OR_THEN_INVERT") == 0) return lexgine::core::BlendLogicalOperation::or_then_invert;

    return lexgine::core::BlendLogicalOperation::no_operation;
}

bool isFillModeAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "SOLID") == 0
        || std::strcmp(attribute.value(), "WIREFRAME") == 0;
}

lexgine::core::FillMode getFillModeFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "SOLID") == 0) return lexgine::core::FillMode::solid;
    if (std::strcmp(attribute.value(), "WIREFRAME") == 0) return lexgine::core::FillMode::wireframe;

    return lexgine::core::FillMode::solid;
}

bool isFaceAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "FRONT") == 0
        || std::strcmp(attribute.value(), "BACK") == 0
        || std::strcmp(attribute.value(), "NODE") == 0;
}

lexgine::core::CullMode getFaceFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "FRONT") == 0) return lexgine::core::CullMode::front;
    if (std::strcmp(attribute.value(), "BACK") == 0) return lexgine::core::CullMode::back;
    if (std::strcmp(attribute.value(), "NODE") == 0) return lexgine::core::CullMode::none;

    return lexgine::core::CullMode::back;
}

bool isWindingModeAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "CLOCKWISE") == 0
        || std::strcmp(attribute.value(), "COUNTERCLOCKWISE") == 0;
}

lexgine::core::FrontFaceWinding getWindingModeFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "CLOCKWISE") == 0) return lexgine::core::FrontFaceWinding::clockwise;
    if (std::strcmp(attribute.value(), "COUNTERCLOCKWISE") == 0) return lexgine::core::FrontFaceWinding::counterclockwise;

    return lexgine::core::FrontFaceWinding::counterclockwise;
}

bool isComparisonFunctionAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "NEVER") == 0
        || std::strcmp(attribute.value(), "LESS") == 0
        || std::strcmp(attribute.value(), "EQUAL") == 0
        || std::strcmp(attribute.value(), "LESS_OR_EQUAL") == 0
        || std::strcmp(attribute.value(), "GREATER") == 0
        || std::strcmp(attribute.value(), "NOT_EQUAL") == 0
        || std::strcmp(attribute.value(), "GREATER_EQUAL") == 0
        || std::strcmp(attribute.value(), "ALWAYS") == 0;
}

lexgine::core::ComparisonFunction getComparisonFunctionFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "NEVER") == 0) return lexgine::core::ComparisonFunction::never;
    if (std::strcmp(attribute.value(), "LESS") == 0) return lexgine::core::ComparisonFunction::less;
    if (std::strcmp(attribute.value(), "EQUAL") == 0) return lexgine::core::ComparisonFunction::equal;
    if (std::strcmp(attribute.value(), "LESS_OR_EQUAL") == 0) return lexgine::core::ComparisonFunction::less_or_equal;
    if (std::strcmp(attribute.value(), "GREATER") == 0) return lexgine::core::ComparisonFunction::greater;
    if (std::strcmp(attribute.value(), "NOT_EQUAL") == 0) return lexgine::core::ComparisonFunction::not_equal;
    if (std::strcmp(attribute.value(), "GREATER_EQUAL") == 0) return lexgine::core::ComparisonFunction::greater_equal;
    if (std::strcmp(attribute.value(), "ALWAYS") == 0) return lexgine::core::ComparisonFunction::always;
    
    return lexgine::core::ComparisonFunction::always;
}

bool isStencilOperationAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "KEEP") == 0
        || std::strcmp(attribute.value(), "ZERO") == 0
        || std::strcmp(attribute.value(), "REPLACE") == 0
        || std::strcmp(attribute.value(), "INCREMENT_AND_SATURATE") == 0
        || std::strcmp(attribute.value(), "DECREMENT_AND_SATURATE") == 0
        || std::strcmp(attribute.value(), "INVERT") == 0
        || std::strcmp(attribute.value(), "INCREMENT") == 0
        || std::strcmp(attribute.value(), "DECREMENT") == 0;
}

lexgine::core::StencilOperation getStencilOperationFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "KEEP") == 0) return lexgine::core::StencilOperation::keep;
    if (std::strcmp(attribute.value(), "ZERO") == 0) return lexgine::core::StencilOperation::zero;
    if (std::strcmp(attribute.value(), "REPLACE") == 0) return lexgine::core::StencilOperation::replace;
    if (std::strcmp(attribute.value(), "INCREMENT_AND_SATURATE") == 0) return lexgine::core::StencilOperation::increment_and_saturate;
    if (std::strcmp(attribute.value(), "DECREMENT_AND_SATURATE") == 0) return lexgine::core::StencilOperation::decrement_and_saturate;
    if (std::strcmp(attribute.value(), "INVERT") == 0) return lexgine::core::StencilOperation::invert;
    if (std::strcmp(attribute.value(), "INCREMENT") == 0) return lexgine::core::StencilOperation::increment;
    if (std::strcmp(attribute.value(), "DECREMENT") == 0) return lexgine::core::StencilOperation::decrement;

    return lexgine::core::StencilOperation::keep;
}

bool isDepthStencilFormatAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "DXGI_FORMAT_D32_FLOAT_S8X24_UINT") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_D32_FLOAT") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_D24_UNORM_S8_UINT") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_D16_UNORM") == 0;
}

DXGI_FORMAT getDepthStencilFormatFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_D32_FLOAT_S8X24_UINT") == 0) return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_D32_FLOAT") == 0) return DXGI_FORMAT_D32_FLOAT;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_D24_UNORM_S8_UINT") == 0) return DXGI_FORMAT_D24_UNORM_S8_UINT;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_D16_UNORM") == 0) return DXGI_FORMAT_D16_UNORM;

    return DXGI_FORMAT_D24_UNORM_S8_UINT;
}

bool isRenderTargetFormatAttribute(pugi::xml_attribute& attribute)
{
    return std::strcmp(attribute.value(), "DXGI_FORMAT_R32G32B32A32_FLOAT") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16B16A16_FLOAT") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16B16A16_UNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16B16A16_SNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R32G32_FLOAT") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R10G10B10A2_UNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R11G11B10_FLOAT") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R8G8B8A8_UNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R8G8B8A8_SNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16_FLOAT") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16_UNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16_SNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R32_FLOAT") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R8G8_UNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R8G8_SNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R16_FLOAT") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R16_UNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R16_SNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R8_UNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_R8_SNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_A8_UNORM") == 0
        || std::strcmp(attribute.value(), "DXGI_FORMAT_B5G6R5_UNORM") == 0;
}

DXGI_FORMAT getRenderTargetFormatFromAttribute(pugi::xml_attribute& attribute)
{
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R32G32B32A32_FLOAT") == 0) return DXGI_FORMAT_R32G32B32A32_FLOAT;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16B16A16_FLOAT") == 0) return DXGI_FORMAT_R16G16B16A16_FLOAT;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16B16A16_UNORM") == 0) return DXGI_FORMAT_R16G16B16A16_UNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16B16A16_SNORM") == 0) return DXGI_FORMAT_R16G16B16A16_SNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R32G32_FLOAT") == 0) return DXGI_FORMAT_R32G32_FLOAT;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R10G10B10A2_UNORM") == 0) return DXGI_FORMAT_R10G10B10A2_UNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R11G11B10_FLOAT") == 0) return DXGI_FORMAT_R11G11B10_FLOAT;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R8G8B8A8_UNORM") == 0) return DXGI_FORMAT_R8G8B8A8_UNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB") == 0) return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R8G8B8A8_SNORM") == 0) return DXGI_FORMAT_R8G8B8A8_SNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16_FLOAT") == 0) return DXGI_FORMAT_R16G16_FLOAT;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16_UNORM") == 0) return DXGI_FORMAT_R16G16_UNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R16G16_SNORM") == 0) return DXGI_FORMAT_R16G16_SNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R32_FLOAT") == 0) return DXGI_FORMAT_R32_FLOAT;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R8G8_UNORM") == 0) return DXGI_FORMAT_R8G8_UNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R8G8_SNORM") == 0) return DXGI_FORMAT_R8G8_SNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R16_FLOAT") == 0) return DXGI_FORMAT_R16_FLOAT;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R16_UNORM") == 0) return DXGI_FORMAT_R16_UNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R16_SNORM") == 0) return DXGI_FORMAT_R16_SNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R8_UNORM") == 0) return DXGI_FORMAT_R8_UNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_R8_SNORM") == 0) return DXGI_FORMAT_R8_SNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_A8_UNORM") == 0) return DXGI_FORMAT_A8_UNORM;
    if (std::strcmp(attribute.value(), "DXGI_FORMAT_B5G6R5_UNORM") == 0) return DXGI_FORMAT_B5G6R5_UNORM;
}

template<attribute_type _type> 
struct attribute_format_to_cpp_format;

template<>
struct attribute_format_to_cpp_format<attribute_type::boolean>
{
    using value_type = bool;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isBooleanAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getBooleanFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::primitive_topology>
{
    using value_type = lexgine::core::PrimitiveTopology;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isPrimitiveTopologyAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getPrimitiveTopologyFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::unsigned_numeric>
{
    using value_type = uint32_t;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isUnsignedNumericAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getUnsignedNumericFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::blend_factor>
{
    using value_type = lexgine::core::BlendFactor;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isBlendFactorAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getBlendFactorFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::blend_operation>
{
    using value_type = lexgine::core::BlendOperation;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isBlendOperationAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getBlendOperationFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::blend_logical_operation>
{
    using value_type = lexgine::core::BlendLogicalOperation;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isBlendLogicalOperationAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getBlendLogicalOperationFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::fill_mode>
{
    using value_type = lexgine::core::FillMode;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isFillModeAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getFillModeFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::face>
{
    using value_type = lexgine::core::CullMode;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isFaceAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getFaceFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::winding_mode>
{
    using value_type = lexgine::core::FrontFaceWinding;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isWindingModeAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getWindingModeFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::comparison_function>
{
    using value_type = lexgine::core::ComparisonFunction;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isComparisonFunctionAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getComparisonFunctionFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::stencil_operation>
{
    using value_type = lexgine::core::StencilOperation;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isStencilOperationAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getStencilOperationFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::depth_stencil_format>
{
    using value_type = DXGI_FORMAT;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isDepthStencilFormatAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getDepthStencilFormatFromAttribute;
};

template<>
struct attribute_format_to_cpp_format<attribute_type::render_target_format>
{
    using value_type = DXGI_FORMAT;
    bool(*is_correct_format_func)(pugi::xml_attribute&) = isRenderTargetFormatAttribute;
    value_type(*extract_attribute_funct)(pugi::xml_attribute&) = getRenderTargetFormatFromAttribute;
};

template<attribute_type _type>
attribute_format_to_cpp_format<_type>::value_type extractAttribute(pugi::xml_attribute& attribute,
    attribute_format_to_cpp_format<_type>::value_type default_value, 
    bool* was_successful)
{
    
}


}


class lexgine::core::dx::d3d12::D3D12PSOXMLParser::impl
{
public:
    impl(D3D12PSOXMLParser& parent) :
        m_parent{ parent }
    {

    }

    void parseAndAddToCompilationCacheShader(pugi::xml_node& node, char const* p_stage_name, bool is_obligatory_shader_stage)
    {
        // parse and attempt to compile vertex shader
        auto shader_node = node.find_child([p_stage_name](pugi::xml_node& n) -> bool
        {
            return strcmp(n.name(), p_stage_name) == 0;
        });

        if (is_obligatory_shader_stage && shader_node.empty())
        {
            m_parent.raiseError(R"*(Error while parsing D3D12 PSO XML description from source ")*"
                + m_parent.m_source_xml + R"*(": obligatory attribute )*" + p_stage_name + " is not found");
            return;
        }

        pugi::char_t const* shader_source_location = shader_node.child_value();

        tasks::ShaderType shader_type;
        char* compilation_task_suffix = "";
        if (strcmp(p_stage_name, "VertexShader") == 0)
        {
            shader_type = tasks::ShaderType::vertex;
            compilation_task_suffix = "VS";
        }
        else if (strcmp(p_stage_name, "HullShader") == 0)
        {
            shader_type = tasks::ShaderType::hull;
            compilation_task_suffix = "HS";
        }
        else if (strcmp(p_stage_name, "DomainShader") == 0)
        {
            shader_type = tasks::ShaderType::domain;
            compilation_task_suffix = "DS";
        }
        else if (strcmp(p_stage_name, "GeometryShader") == 0)
        {
            shader_type = tasks::ShaderType::geometry;
            compilation_task_suffix = "GS";
        }
        else if (strcmp(p_stage_name, "PixelShader") == 0)
        {
            shader_type = tasks::ShaderType::pixel;
            compilation_task_suffix = "PS";
        }

        pugi::char_t const* shader_entry_point_name = shader_node.attribute("entry").as_string();

        m_parent.m_hlsl_compilation_task_cache.addTask(shader_source_location, m_currently_assembled_pso_descriptor.name + "_"
            + shader_source_location + "_" + compilation_task_suffix, shader_type, shader_entry_point_name,
            lexgine::core::ShaderSourceCodePreprocessor::SourceType::file);
    }

    void parseGraphicsPSO(pugi::xml_node& node)
    {
        // Get attributes of the graphics PSO
        m_currently_assembled_pso_descriptor.name = node.attribute("name").as_string(("GraphicsPSO_" + m_parent.getId().toString()).c_str());
        m_currently_assembled_pso_descriptor.pso_type = dx::d3d12::PSOType::graphics;



        // Retrieve shader stages
        parseAndAddToCompilationCacheShader(node, "VertexShader", true);
        parseAndAddToCompilationCacheShader(node, "HullShader", false);
        parseAndAddToCompilationCacheShader(node, "DomainShader", false);
        parseAndAddToCompilationCacheShader(node, "GeometryShader", false);
        parseAndAddToCompilationCacheShader(node, "PixelShader", true);


    }

    void parseComputePSO(pugi::xml_node& node)
    {

    }

private:
    D3D12PSOXMLParser& m_parent;
    PSOIntermediateDescriptor m_currently_assembled_pso_descriptor;
};


lexgine::core::dx::d3d12::D3D12PSOXMLParser::D3D12PSOXMLParser(std::string const& xml_source, bool deferred_shader_compilation) :
    m_source_xml{ xml_source },
    m_deferred_shader_compilation{ deferred_shader_compilation },
    m_impl{ new impl{*this} }
{
    pugi::xml_document xml_doc;
    pugi::xml_parse_result parse_result = xml_doc.load_string(xml_source.c_str());

    if (parse_result)
    {
        for (pugi::xml_node_iterator it : xml_doc)
        {
            if (std::strcmp(it->name(), "GraphicsPSO") == 0)
                m_impl->parseGraphicsPSO(*it);
            else if (std::strcmp(it->name(), "ComputePSO") == 0)
                m_impl->parseComputePSO(*it);
            else
            {
                ErrorBehavioral::raiseError(std::string{ R"*(Unknown attribute ")*" }
                +it->name() + R"*(" while parsing D3D12 PSO XML description from file ")*" + xml_source + R"*(")*");
                return;
            }
        }
    }
    else
    {
        ErrorBehavioral::raiseError(
            std::string{ "Unable to parse XML source\n"
            "Reason: " } + parse_result.description() + "\n"
            "Location: " + std::string{ xml_source.c_str() + parse_result.offset, std::min<size_t>(xml_source.size() - parse_result.offset, 80) - 1 }
        );
    }
}

lexgine::core::dx::d3d12::D3D12PSOXMLParser::~D3D12PSOXMLParser() = default;

lexgine::core::dx::d3d12::D3D12PSOXMLParser::PSOIntermediateDescriptor::PSOIntermediateDescriptor() :
    pso_type{ dx::d3d12::PSOType::graphics },
    graphics{}
{

}

D3D12PSOXMLParser::PSOIntermediateDescriptor::~PSOIntermediateDescriptor()
{
    switch (pso_type)
    {
    case dx::d3d12::PSOType::graphics:
        graphics.~GraphicsPSODescriptor();
        break;
    case d3d12::PSOType::compute:
        compute.~ComputePSODescriptor();
        break;
    }
}

D3D12PSOXMLParser::PSOIntermediateDescriptor& D3D12PSOXMLParser::PSOIntermediateDescriptor::operator=(PSOIntermediateDescriptor const& other)
{
    if (this == &other)
        return *this;

    name = other.name;
    pso_type = other.pso_type;

    if (pso_type == dx::d3d12::PSOType::graphics &&
        other.pso_type == dx::d3d12::PSOType::graphics)
    {
        graphics = other.graphics;
    }
    else if (pso_type == dx::d3d12::PSOType::compute &&
        other.pso_type == dx::d3d12::PSOType::compute)
    {
        compute = other.compute;
    }
    else if (pso_type == dx::d3d12::PSOType::graphics &&
        other.pso_type == dx::d3d12::PSOType::compute)
    {
        graphics.~GraphicsPSODescriptor();
        compute = other.compute;
    }
    else if (pso_type == dx::d3d12::PSOType::compute &&
        other.pso_type == dx::d3d12::PSOType::graphics)
    {
        compute.~ComputePSODescriptor();
        graphics = other.graphics;
    }

    return *this;
}

D3D12PSOXMLParser::PSOIntermediateDescriptor& D3D12PSOXMLParser::PSOIntermediateDescriptor::operator=(PSOIntermediateDescriptor&& other)
{
    if (this == &other)
        return *this;

    name = std::move(other.name);
    pso_type = std::move(other.pso_type);

    if (pso_type == dx::d3d12::PSOType::graphics &&
        other.pso_type == dx::d3d12::PSOType::graphics)
    {
        graphics = std::move(other.graphics);
    }
    else if (pso_type == dx::d3d12::PSOType::compute &&
        other.pso_type == dx::d3d12::PSOType::compute)
    {
        compute = std::move(other.compute);
    }
    else if (pso_type == dx::d3d12::PSOType::graphics &&
        other.pso_type == dx::d3d12::PSOType::compute)
    {
        graphics.~GraphicsPSODescriptor();
        compute = std::move(other.compute);
    }
    else if (pso_type == dx::d3d12::PSOType::compute &&
        other.pso_type == dx::d3d12::PSOType::graphics)
    {
        compute.~ComputePSODescriptor();
        graphics = std::move(other.graphics);
    }

    return *this;
}
