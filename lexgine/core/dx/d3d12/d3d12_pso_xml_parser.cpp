#include <algorithm>
#include <regex>
#include <limits>

#include "d3d12_pso_xml_parser.h"
#include "d3d12_tools.h"
#include "../../misc/template_argument_iterator.h"

#include "pugixml.hpp"


using namespace lexgine::core::dx::d3d12;


namespace {

    enum class attribute_type {
        boolean,
        primitive_topology,
        unsigned_numeric,
        list_of_unsigned_numerics,
        blend_factor,
        blend_operation,
        blend_logical_operation,
        fill_mode,
        face,
        winding_mode,
        comparison_function,
        stencil_operation,
        depth_stencil_format,
        data_format,
        render_target_format,
        string,
        floating_point
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

    bool isListOfUnsignedNumerics(pugi::xml_attribute& attribute)
    {
        std::regex list_of_unsigned_numeric_regular_expression{ R"?(\s*[0-9]+(\s*,\s*[0-9]+)*\s*)?" };
        return std::regex_match(attribute.value(), list_of_unsigned_numeric_regular_expression, std::regex_constants::match_not_null);
    }

    std::list<uint32_t> getListOfUnsignedNumericsFromArgument(pugi::xml_attribute& attribute)
    {
        std::list<uint32_t> rv{};
        std::string source_string{ attribute.value() };
        int i = 0;
        while (i < source_string.length())
        {
            while (i < source_string.length() && source_string[i] < '0' && source_string[i] > '9') ++i;

            std::string numeric_value;
            while (i < source_string.length() && source_string[i] >= '0' && source_string[i] <= '9')
            {
                numeric_value += source_string[i];
                ++i;
            }
            if (!numeric_value.empty())
            {
                rv.push_back(static_cast<uint32_t>(std::stoul(numeric_value)));
                source_string = source_string.substr(i);
            }
        }

        return rv;
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
        if (std::strcmp(attribute.value(), "OR") == 0) return lexgine::core::BlendLogicalOperation:: or ;
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

    bool isDataFormatAttribute(pugi::xml_attribute& attribute)
    {
        return std::strcmp(attribute.value(), "FLOAT32") == 0
            || std::strcmp(attribute.value(), "FLOAT16") == 0
            || std::strcmp(attribute.value(), "INT32") == 0
            || std::strcmp(attribute.value(), "INT16") == 0
            || std::strcmp(attribute.value(), "UINT32") == 0
            || std::strcmp(attribute.value(), "UINT16") == 0;
    }

    lexgine::core::misc::DataFormat getDataFormatFromAttribute(pugi::xml_attribute& attribute)
    {
        if (std::strcmp(attribute.value(), "FLOAT32") == 0) return lexgine::core::misc::DataFormat::float32;
        if (std::strcmp(attribute.value(), "FLOAT16") == 0) return lexgine::core::misc::DataFormat::float16;
        if (std::strcmp(attribute.value(), "INT32") == 0) return lexgine::core::misc::DataFormat::int32;
        if (std::strcmp(attribute.value(), "INT16") == 0) return lexgine::core::misc::DataFormat::int16;
        if (std::strcmp(attribute.value(), "UINT32") == 0) return lexgine::core::misc::DataFormat::uint32;
        if (std::strcmp(attribute.value(), "UINT16") == 0) return lexgine::core::misc::DataFormat::uint16;

        return lexgine::core::misc::DataFormat::unknown;
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

        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    }

    bool isStringAttribute(pugi::xml_attribute& attribute)
    {
        return !attribute.empty();
    }

    std::string getStringFromAttribute(pugi::xml_attribute& attribute)
    {
        return attribute.value();
    }

    bool isFloatingPointAttribute(pugi::xml_attribute& attribute)
    {
        return !std::isnan(attribute.as_float(std::numeric_limits<float>::quiet_NaN()));
    }

    float getFloatingPointFromAttribute(pugi::xml_attribute& attribute)
    {
        return attribute.as_float(std::numeric_limits<float>::quiet_NaN());
    }


    template<attribute_type _type>
    struct attribute_format_to_cpp_format;

    template<>
    struct attribute_format_to_cpp_format<attribute_type::boolean>
    {
        using value_type = bool;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isBooleanAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getBooleanFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::primitive_topology>
    {
        using value_type = lexgine::core::PrimitiveTopology;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isPrimitiveTopologyAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getPrimitiveTopologyFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::unsigned_numeric>
    {
        using value_type = uint32_t;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isUnsignedNumericAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getUnsignedNumericFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::list_of_unsigned_numerics>
    {
        using value_type = std::list<uint32_t>;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isListOfUnsignedNumerics;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getListOfUnsignedNumericsFromArgument;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::blend_factor>
    {
        using value_type = lexgine::core::BlendFactor;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isBlendFactorAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getBlendFactorFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::blend_operation>
    {
        using value_type = lexgine::core::BlendOperation;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isBlendOperationAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getBlendOperationFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::blend_logical_operation>
    {
        using value_type = lexgine::core::BlendLogicalOperation;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isBlendLogicalOperationAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getBlendLogicalOperationFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::fill_mode>
    {
        using value_type = lexgine::core::FillMode;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isFillModeAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getFillModeFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::face>
    {
        using value_type = lexgine::core::CullMode;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isFaceAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getFaceFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::winding_mode>
    {
        using value_type = lexgine::core::FrontFaceWinding;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isWindingModeAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getWindingModeFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::comparison_function>
    {
        using value_type = lexgine::core::ComparisonFunction;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isComparisonFunctionAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getComparisonFunctionFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::stencil_operation>
    {
        using value_type = lexgine::core::StencilOperation;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isStencilOperationAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getStencilOperationFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::depth_stencil_format>
    {
        using value_type = DXGI_FORMAT;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isDepthStencilFormatAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getDepthStencilFormatFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::data_format>
    {
        using value_type = lexgine::core::misc::DataFormat;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isDataFormatAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getDataFormatFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::render_target_format>
    {
        using value_type = DXGI_FORMAT;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isRenderTargetFormatAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getRenderTargetFormatFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::string>
    {
        using value_type = std::string;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isStringAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getStringFromAttribute;
    };

    template<>
    struct attribute_format_to_cpp_format<attribute_type::floating_point>
    {
        using value_type = float;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isFloatingPointAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getFloatingPointFromAttribute;
    };


    template<attribute_type _type>
    typename attribute_format_to_cpp_format<_type>::value_type extractAttribute(pugi::xml_attribute& attribute,
        typename attribute_format_to_cpp_format<_type>::value_type default_value,
        bool* was_successful = nullptr)
    {
        if (attribute_format_to_cpp_format<_type>::is_correct_format_func(attribute))
        {
            // attribute has expected format

            if (was_successful) *was_successful = true;
            return attribute_format_to_cpp_format<_type>::extract_attribute_func(attribute);
        }
        else
        {
            // attribute has wrong format or was not found at all, return specified default value

            if (was_successful) *was_successful = false;
            return default_value;
        }
    }


    class LoopBodyCommonPart
    {
    protected:
        static std::shared_ptr<lexgine::core::AbstractVertexAttributeSpecification> m_va_specification;

    public:
        static lexgine::core::misc::DataFormat type;
        static unsigned char size;
        static bool is_normalized;
        static unsigned char primitive_assembler_input_slot;
        static char const* name;
        static uint32_t name_index;
        static uint32_t instancing_data_rate;

    public:
        using arg_pack0 = lexgine::core::misc::arg_pack<float, int16_t, int32_t, uint16_t, uint32_t>;
        using value_arg_pack0 = lexgine::core::misc::value_arg_pack<unsigned char, 1U, 2U, 3U, 4U>;
        using value_arg_pack1 = lexgine::core::misc::value_arg_pack<bool, false, true>;
        using value_arg_pack2 = lexgine::core::misc::value_arg_pack<bool, false, true>;
        
        static std::shared_ptr<lexgine::core::AbstractVertexAttributeSpecification> getVASpecification() { return m_va_specification; }
    };
    std::shared_ptr<lexgine::core::AbstractVertexAttributeSpecification> LoopBodyCommonPart::m_va_specification{ nullptr };
    lexgine::core::misc::DataFormat LoopBodyCommonPart::type{ lexgine::core::misc::DataFormat::unknown };
    unsigned char LoopBodyCommonPart::size{ 0U };
    bool LoopBodyCommonPart::is_normalized{ false };
    unsigned char LoopBodyCommonPart::primitive_assembler_input_slot{ 0U };
    char const* LoopBodyCommonPart::name{ nullptr };
    uint32_t LoopBodyCommonPart::name_index{ 0U };
    uint32_t LoopBodyCommonPart::instancing_data_rate{ 0U };



    template<typename TupleListType>
    class LoopBody : public LoopBodyCommonPart
    {
    public:
        static bool iterate()
        {
            using arg0 = typename lexgine::core::misc::get_tuple_element<TupleListType, 0>::value_type;
            constexpr auto arg1 = lexgine::core::misc::get_tuple_element<TupleListType, 1>::value;
            constexpr auto arg2 = lexgine::core::misc::get_tuple_element<TupleListType, 2>::value;
            constexpr auto arg3 = lexgine::core::misc::get_tuple_element<TupleListType, 3>::value;

            if ((type == lexgine::core::dx::d3d12::StaticTypeToDataFormat<arg0>::data_format ||
                lexgine::core::dx::d3d12::StaticTypeToDataFormat<arg0>::data_format == lexgine::core::misc::DataFormat::float32 &&
                type == lexgine::core::misc::DataFormat::float16 && arg3 == true) &&
                size == arg1 && is_normalized == arg2)
            {
                m_va_specification.reset(
                    new lexgine::core::VertexAttributeSpecification<
                        arg0,
                        arg1,
                        arg2,
                        arg3>{ primitive_assembler_input_slot, name, name_index, instancing_data_rate });
                return false;
            }

            return true;
        }
        
        static std::unique_ptr<lexgine::core::AbstractVertexAttributeSpecification> getVASpecification() { return m_va_specification; }
    };

    std::shared_ptr<lexgine::core::AbstractVertexAttributeSpecification> createVertexAttributeSpecification(lexgine::core::misc::DataFormat type, uint32_t size, bool is_normalized,
        unsigned char primitive_assembler_input_slot, char const* name, uint32_t name_index, uint32_t instancing_data_rate)
    {
        LoopBodyCommonPart::type = type;
        LoopBodyCommonPart::size = size;
        LoopBodyCommonPart::is_normalized = is_normalized;
        LoopBodyCommonPart::primitive_assembler_input_slot = primitive_assembler_input_slot;
        LoopBodyCommonPart::name = name;
        LoopBodyCommonPart::name_index = name_index;
        LoopBodyCommonPart::instancing_data_rate = instancing_data_rate;
        lexgine::core::misc::TemplateArgumentIterator<LoopBody, LoopBodyCommonPart::arg_pack0, LoopBodyCommonPart::value_arg_pack0, LoopBodyCommonPart::value_arg_pack1, LoopBodyCommonPart::value_arg_pack2>::loop();

        return LoopBodyCommonPart::getVASpecification();
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
        m_currently_assembled_pso_descriptor.graphics.primitive_restart = extractAttribute<attribute_type::boolean>(node.attribute("primitive_restart"), false);
        m_currently_assembled_pso_descriptor.graphics.primitive_topology = extractAttribute<attribute_type::primitive_topology>(node.attribute("primitive_topology"), lexgine::core::PrimitiveTopology::triangle);
        m_currently_assembled_pso_descriptor.graphics.sample_mask = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("sample_mask"), 0xFFFFFFFF);
        m_currently_assembled_pso_descriptor.graphics.multi_sampling_format.count = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("sample_count"), 1);
        m_currently_assembled_pso_descriptor.graphics.multi_sampling_format.quality = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("sample_quality"), 1);


        // Retrieve shader stages
        parseAndAddToCompilationCacheShader(node, "VertexShader", true);
        parseAndAddToCompilationCacheShader(node, "HullShader", false);
        parseAndAddToCompilationCacheShader(node, "DomainShader", false);
        parseAndAddToCompilationCacheShader(node, "GeometryShader", false);
        parseAndAddToCompilationCacheShader(node, "PixelShader", true);


        // Read stream output descriptor if present
        {
            auto stream_output_descriptor_node = node.find_child([](pugi::xml_node& n) -> bool
            {
                return std::strcmp(n.name(), "StreamOutput") == 0;
            });
            if (!stream_output_descriptor_node.empty())
            {
                // stream output descriptor is present, continue parsing
                bool is_attribute_present{ false };

                auto rasterized_stream_attribute = stream_output_descriptor_node.attribute("rasterized_stream");
                uint32_t rasterized_stream = isNullAttribute(rasterized_stream_attribute) ? lexgine::core::misc::NoRasterizationStream :
                    extractAttribute<attribute_type::unsigned_numeric>(rasterized_stream_attribute, 0);
                std::list<uint32_t> buffer_strides = extractAttribute<attribute_type::list_of_unsigned_numerics>(stream_output_descriptor_node.attribute("strides"),
                    std::list<uint32_t>{}, &is_attribute_present);
                if (!is_attribute_present)
                {
                    m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + ": stream output descriptor must define attribute \"strides\"");
                    return;
                }

                std::list<lexgine::core::StreamOutputDeclarationEntry> so_entry_descs;
                for (auto& node : stream_output_descriptor_node)
                {
                    if (std::strcmp(node.name(), "DeclarationEntry") == 0)
                    {
                        uint32_t stream = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("stream"), 0);
                        std::string name = extractAttribute<attribute_type::string>(node.attribute("name"), "OUTPUT_STREAM", &is_attribute_present);
                        if (!is_attribute_present)
                        {
                            m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + ": attribute \"name\" must be defined by \"DeclarationEntry\"");
                            return;
                        }
                        char const* processed_name = name == "NULL" ? nullptr : name.c_str();
                        uint32_t name_index = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("name_index"), 0);
                        uint32_t start_component = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("start_component"), 0, &is_attribute_present);
                        if (!is_attribute_present)
                        {
                            m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + ": attribute \"start_component\" must be defined by \"DeclaraionEntry\"");
                            return;
                        }
                        uint32_t component_count = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("component_count"), 4, &is_attribute_present);
                        if (!is_attribute_present)
                        {
                            m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + ": attribute \"component_count\" must be defined by \"DeclaraionEntry\"");
                            return;
                        }
                        uint32_t slot = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("slot"), 0, &is_attribute_present);
                        if (!is_attribute_present)
                        {
                            m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + ": attribute \"start_component\" must be defined by \"DeclaraionEntry\"");
                            return;
                        }

                        so_entry_descs.emplace_back(stream, name.c_str(), name_index, start_component, component_count, slot);
                    }
                }

                m_currently_assembled_pso_descriptor.graphics.stream_output = lexgine::core::StreamOutput{ so_entry_descs, buffer_strides, rasterized_stream };
            }
        }

        // Read blending stage declaration if present
        {
            auto blend_state_node = node.find_child([](pugi::xml_node& n) -> bool
            {
                return std::strcmp(n.name(), "BlendState") == 0;
            });

            if (!blend_state_node.empty())
            {
                // Blend state is present, read contents
                bool alpha_to_coverage = extractAttribute<attribute_type::boolean>(blend_state_node.attribute("alpha_to_coverage"), false);
                bool independent_blending = extractAttribute<attribute_type::boolean>(blend_state_node.attribute("independent_blending"), false);

                m_currently_assembled_pso_descriptor.graphics.blend_state = lexgine::core::BlendState{ alpha_to_coverage, independent_blending };
                
                for (auto& blend_desc_node : blend_state_node)
                {
                    if (std::strcmp(blend_desc_node.name(), "BlendDescriptor") == 0)
                    {
                        bool is_attribute_present{ false };

                        uint8_t render_target =
                            static_cast<uint8_t>(extractAttribute<attribute_type::unsigned_numeric>(blend_desc_node.attribute("render_target"), 0, &is_attribute_present));
                        if (!is_attribute_present)
                        {
                            m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + ": BlendDescriptor must have attribute \"render_target\"");
                            return;
                        }

                        bool enable_blending = extractAttribute<attribute_type::boolean>(blend_desc_node.attribute("enable_blending"), true);
                        bool enable_logic_operation = extractAttribute<attribute_type::boolean>(blend_desc_node.attribute("enable_logic_operation"), false);

                        lexgine::core::BlendFactor source_blend_factor =
                            extractAttribute<attribute_type::blend_factor>(blend_desc_node.attribute("source_blend_factor"), lexgine::core::BlendFactor::one);
                        lexgine::core::BlendFactor destination_blend_factor =
                            extractAttribute<attribute_type::blend_factor>(blend_desc_node.attribute("destination_blend_factor"), lexgine::core::BlendFactor::zero);
                        lexgine::core::BlendFactor source_alpha_blend_factor =
                            extractAttribute<attribute_type::blend_factor>(blend_desc_node.attribute("source_alpha_blend_factor"), lexgine::core::BlendFactor::one);
                        lexgine::core::BlendFactor destination_alpha_blend_factor =
                            extractAttribute<attribute_type::blend_factor>(blend_desc_node.attribute("destination_alpha_blend_factor"), lexgine::core::BlendFactor::zero);

                        lexgine::core::BlendOperation blend_op =
                            extractAttribute<attribute_type::blend_operation>(blend_desc_node.attribute("blend_operation"), lexgine::core::BlendOperation::add);
                        lexgine::core::BlendOperation alpha_blend_op =
                            extractAttribute<attribute_type::blend_operation>(blend_desc_node.attribute("alpha_blend_operation"), lexgine::core::BlendOperation::add);

                        lexgine::core::BlendLogicalOperation blend_logical_op =
                            extractAttribute<attribute_type::blend_logical_operation>(blend_desc_node.attribute("logical_operation"), lexgine::core::BlendLogicalOperation::no_operation);

                        uint8_t color_mask =
                            static_cast<uint8_t>(extractAttribute<attribute_type::unsigned_numeric>(blend_desc_node.attribute("color_mask"), 0xF));

                        m_currently_assembled_pso_descriptor.graphics.blend_state.render_target_blend_descriptor[render_target] = lexgine::core::BlendDescriptor{
                            source_blend_factor, source_alpha_blend_factor, destination_blend_factor, destination_alpha_blend_factor,
                            blend_op, alpha_blend_op, enable_logic_operation, blend_logical_op, color_mask };
                    }

                }
            }
        }

        // Read rasterization descriptor if present
        {
            auto rasterization_desc_node = node.find_child([](pugi::xml_node& n) -> bool
            {
                return std::strcmp(n.name(), "RasterizerDescriptor") == 0;
            });

            if (!rasterization_desc_node.empty())
            {
                // Rasterization descriptor has been found, proceed to read the contents
                
                lexgine::core::FillMode fill_mode = extractAttribute<attribute_type::fill_mode>(rasterization_desc_node.attribute("fill_mode"), lexgine::core::FillMode::solid);
                lexgine::core::CullMode cull_mode = extractAttribute<attribute_type::face>(rasterization_desc_node.attribute("cull_mode"), lexgine::core::CullMode::back);
                lexgine::core::FrontFaceWinding front_face_winding = extractAttribute<attribute_type::winding_mode>(rasterization_desc_node.attribute("front_face_winding"), 
                    lexgine::core::FrontFaceWinding::counterclockwise);
                int depth_bias = extractAttribute<attribute_type::unsigned_numeric>(rasterization_desc_node.attribute("depth_bias"), 0);
                float depth_bias_clamp = extractAttribute<attribute_type::floating_point>(rasterization_desc_node.attribute("depth_bias_clamp"), 0.f);
                float slope_scaled_depth_bias = extractAttribute<attribute_type::floating_point>(rasterization_desc_node.attribute("slope_scaled_depth_bias"), 0.f);
                bool depth_clip = extractAttribute<attribute_type::boolean>(rasterization_desc_node.attribute("depth_clip"), true);
                bool line_anti_aliasing = extractAttribute<attribute_type::boolean>(rasterization_desc_node.attribute("line_anti_aliasing"), false);
                bool multi_sampling = extractAttribute<attribute_type::boolean>(rasterization_desc_node.attribute("multi_sampling"), false);
                bool concervative_rasterization = extractAttribute<attribute_type::boolean>(rasterization_desc_node.attribute("concervative_rasterization"), false);

                m_currently_assembled_pso_descriptor.graphics.rasterization_descriptor =
                    lexgine::core::RasterizerDescriptor{ fill_mode, cull_mode, front_face_winding, depth_bias, depth_bias_clamp, slope_scaled_depth_bias, depth_clip,
                    multi_sampling, line_anti_aliasing, concervative_rasterization ? lexgine::core::ConservativeRasterizationMode::on : lexgine::core::ConservativeRasterizationMode::off };
            }
        }

        // Read depth-stencil descriptor if present
        {
            auto depth_stencil_desc_node = node.find_child([](pugi::xml_node& n) -> bool
            {
                return std::strcmp(n.name(), "DepthStencilDescriptor") == 0;
            });

            if (!depth_stencil_desc_node.empty())
            {
                bool enable_depth_test = extractAttribute<attribute_type::boolean>(depth_stencil_desc_node.attribute("enable_depth_test"), true);
                bool allow_depth_writes = extractAttribute<attribute_type::boolean>(depth_stencil_desc_node.attribute("allow_depth_writes"), true);
                lexgine::core::ComparisonFunction depth_test_comparison_function =
                    extractAttribute<attribute_type::comparison_function>(depth_stencil_desc_node.attribute("depth_test_comparison_function"), lexgine::core::ComparisonFunction::less);
                bool enable_stencil_test = extractAttribute<attribute_type::boolean>(depth_stencil_desc_node.attribute("enable_stencil_test"), false);
                uint32_t stencil_read_mask = extractAttribute<attribute_type::unsigned_numeric>(depth_stencil_desc_node.attribute("stencil_read_mask"), 0xFF);
                uint32_t stencil_write_mask = extractAttribute<attribute_type::unsigned_numeric>(depth_stencil_desc_node.attribute("stencil_write_mask"), 0xFF);


                lexgine::core::StencilBehavior stencil_test_beavior[2];
                for (auto& stencil_test_behavior_node : depth_stencil_desc_node)
                {
                    if (std::strcmp(stencil_test_behavior_node.name(), "StencilTestBehavior") == 0)
                    {
                        bool was_successful{ false };
                        auto front_face = extractAttribute<attribute_type::face>(stencil_test_behavior_node.attribute("face"), lexgine::core::CullMode::front, &was_successful);
                        if (!was_successful)
                        {
                            m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + 
                                ": StencilTestBehavior node must define attribute \"face\"");
                            return;
                        }

                        auto comparison_function = 
                            extractAttribute<attribute_type::comparison_function>(stencil_test_behavior_node.attribute("comparison_function"), lexgine::core::ComparisonFunction::always);
                        auto operation_st_fail = 
                            extractAttribute<attribute_type::stencil_operation>(stencil_test_behavior_node.attribute("operation_st_fail"), lexgine::core::StencilOperation::keep);
                        auto operation_st_pass_dt_fail = 
                            extractAttribute<attribute_type::stencil_operation>(stencil_test_behavior_node.attribute("operation_st_pass_dt_fail"), lexgine::core::StencilOperation::keep);
                        auto operation_st_pass_dt_pass = 
                            extractAttribute<attribute_type::stencil_operation>(stencil_test_behavior_node.attribute("operation_st_pass_dt_pass"), lexgine::core::StencilOperation::keep);
                        DXGI_FORMAT dsv_target_format =
                            extractAttribute<attribute_type::depth_stencil_format>(stencil_test_behavior_node.attribute("dsv_target_format"), DXGI_FORMAT_D32_FLOAT_S8X24_UINT, &was_successful);
                        if (!was_successful)
                        {
                            m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + 
                                ": StencilTestBehavior node must define attribute \"dsv_target_format\"");
                            return;
                        }

                        stencil_test_beavior[front_face == lexgine::core::CullMode::front ? 0 : 1] =
                            lexgine::core::StencilBehavior{ operation_st_fail, operation_st_pass_dt_fail, operation_st_pass_dt_pass, comparison_function };
                    }
                }

                m_currently_assembled_pso_descriptor.graphics.depth_stencil_descriptor = lexgine::core::DepthStencilDescriptor{ enable_depth_test, allow_depth_writes,
                depth_test_comparison_function, enable_stencil_test, stencil_test_beavior[0], stencil_test_beavior[1] };
            }

            

        }

        // Read vertex input attributes
        {
            auto va_specification_node = node.find_child([](pugi::xml_node& n) -> bool
            {
                return std::strcmp(n.name(), "VertexAttributeSpecification") == 0;
            });

            
            unsigned char slot{ 0U };
            for (auto& va : va_specification_node)
            {
                if (std::strcmp(va.name(), "VertexAttribute") == 0)
                {
                    bool was_successful{ false };
                    std::string name = extractAttribute<attribute_type::string>(va.attribute("name"), "", &was_successful);
                    if (!was_successful)
                    {
                        m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + ": VertexAttribute node must define attribute \"name\"");
                        return;
                    }

                    uint32_t index = extractAttribute<attribute_type::unsigned_numeric>(va.attribute("index"), 0, &was_successful);
                    if (!was_successful)
                    {
                        m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + ": VertexAttribute node must define attribute \"index\"");
                        return;
                    }


                    uint32_t size = extractAttribute<attribute_type::unsigned_numeric>(va.attribute("size"), 0, &was_successful);
                    if (!was_successful)
                    {
                        m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + ": VertexAttribute node must define attribute \"size\"");
                        return;
                    }

                    lexgine::core::misc::DataFormat type = extractAttribute<attribute_type::data_format>(va.attribute("type"), lexgine::core::misc::DataFormat::float32, &was_successful);
                    if (!was_successful)
                    {
                        m_parent.raiseError("error parsing XML PSO source of graphics PSO " + m_currently_assembled_pso_descriptor.name + ": VertexAttribute node must define attribute \"type\"");
                        return;
                    }

                    bool normalized = extractAttribute<attribute_type::boolean>(va.attribute("normalized"), false);
                    uint32_t instancing_rate = extractAttribute<attribute_type::unsigned_numeric>(va.attribute("instancing_rate"), 0);

                    m_currently_assembled_pso_descriptor.graphics.vertex_attributes.push_back(createVertexAttributeSpecification(type, size, normalized, slot, name.c_str(), index, instancing_rate));
                }
            }
        }
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
