#include <algorithm>
#include <regex>
#include <limits>
#include <unordered_set>

#include "d3d12_pso_xml_parser.h"
#include "d3d12_tools.h"

#include "lexgine/core/global_settings.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/logging_streams.h"

#include "lexgine/core/misc/template_argument_iterator.h"

#include "lexgine/core/dx/dxcompilation/common.h"
#include "lexgine/core/dx/d3d12/tasks/hlsl_compilation_task.h"
#include "lexgine/core/dx/d3d12/tasks/pso_compilation_task.h"
#include "lexgine/core/dx/d3d12/tasks/root_signature_compilation_task.h"

#include "lexgine/core/concurrency/task_sink.h"

#include "pugixml.hpp"

using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::dxcompilation;


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
        floating_point,
        shader_model
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

    lexgine::core::PrimitiveTopologyType getPrimitiveTopologyFromAttribute(pugi::xml_attribute& attribute)
    {
        if (std::strcmp(attribute.value(), "POINT") == 0) return lexgine::core::PrimitiveTopologyType::point;
        if (std::strcmp(attribute.value(), "LINE") == 0) return lexgine::core::PrimitiveTopologyType::line;
        if (std::strcmp(attribute.value(), "TRIANGLE") == 0) return lexgine::core::PrimitiveTopologyType::triangle;
        if (std::strcmp(attribute.value(), "PATCH") == 0) return lexgine::core::PrimitiveTopologyType::patch;

        return lexgine::core::PrimitiveTopologyType::triangle;
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
            while (i < source_string.length() && (source_string[i] < '0' || source_string[i] > '9')) ++i;

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

            i = 0;
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

    bool isShaderModelAttribute(pugi::xml_attribute& attribute)
    {
        std::string rv = attribute.as_string("5.0");    // default shader model is 5.0

        size_t begin_idx = rv.find_first_not_of(" \t");
        size_t end_idx = rv.find_last_not_of(" \t");

        if (rv[0] < '0' || rv[0] > '9'
            || rv[1] != '.'
            || rv[2] < '0' || rv[2] > '9') return false;

        return true;
    }

    ShaderModel getShaderModelFromAttribute(pugi::xml_attribute& attribute)
    {
        std::string rv = attribute.as_string("5.0");
        size_t begin_idx = rv.find_first_not_of(" \t");
        size_t end_idx = rv.find_last_not_of(" \t");
        rv = rv.substr(begin_idx, end_idx - begin_idx + 1);

        uint8_t major_version = rv[0] - '0';
        uint8_t minor_version = rv[2] - '0';
        unsigned short packed_shader_model = (major_version << 4) | minor_version;

        switch (packed_shader_model)
        {
        case static_cast<unsigned short>(ShaderModel::model_60):
            return ShaderModel::model_60;

        case static_cast<unsigned short>(ShaderModel::model_61):
            return ShaderModel::model_61;

        case static_cast<unsigned short>(ShaderModel::model_62):
            return ShaderModel::model_62;

        case static_cast<unsigned short>(ShaderModel::model_50):
        default:
            return ShaderModel::model_50;
        }
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
        using value_type = lexgine::core::PrimitiveTopologyType;
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

    template<>
    struct attribute_format_to_cpp_format<attribute_type::shader_model>
    {
        using value_type = ShaderModel;
        static constexpr bool(*is_correct_format_func)(pugi::xml_attribute&) = isShaderModelAttribute;
        static constexpr value_type(*extract_attribute_func)(pugi::xml_attribute&) = getShaderModelFromAttribute;
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

    struct VATargetSpecification
    {
        using arg_pack0 = arg_pack<half, float, int16_t, int32_t, uint16_t, uint32_t>;
        using value_arg_pack0 = value_arg_pack<unsigned char, 1U, 2U, 3U, 4U>;
        using value_arg_pack1 = value_arg_pack<bool, false, true>;

        DataFormat va_target_data_format;
        unsigned char va_target_data_format_element_size;
        bool va_is_target_format_normalized;

        unsigned char ia_slot;
        unsigned char element_stride;
        char const* name;
        uint32_t name_index;
        uint32_t instancing_rate;

        std::shared_ptr<AbstractVertexAttributeSpecification> instance;
    };



    template<typename TupleListType>
    class VASpecificationLoopBody
    {
    public:
        static bool iterate(void* user_data)
        {
            using arg0 = typename get_tuple_element<TupleListType, 0>::value_type;
            constexpr auto arg1 = get_tuple_element<TupleListType, 1>::value;
            constexpr auto arg2 = get_tuple_element<TupleListType, 2>::value;

            VATargetSpecification& va_target_spec = *(reinterpret_cast<VATargetSpecification*>(user_data));

            if (va_target_spec.va_target_data_format == StaticTypeToDataFormat<arg0>::data_format 
                && va_target_spec.va_target_data_format_element_size == arg1 
                && va_target_spec.va_is_target_format_normalized == arg2)
            {
                va_target_spec.instance.reset(
                    new VertexAttributeSpecification<
                        arg0,
                        arg1,
                        arg2>{ va_target_spec.ia_slot, va_target_spec.element_stride, va_target_spec.name, va_target_spec.name_index, va_target_spec.instancing_rate });
                return false;
            }

            return true;
        }
    };

    std::shared_ptr<AbstractVertexAttributeSpecification> createVertexAttributeSpecification(DataFormat type, uint32_t size, bool is_normalized,
        unsigned char primitive_assembler_input_slot, unsigned char element_stride, char const* name, uint32_t name_index, uint32_t instancing_data_rate)
    {
        VATargetSpecification target_spec;

        target_spec.va_target_data_format = type;
        target_spec.va_target_data_format_element_size = size;
        target_spec.va_is_target_format_normalized = is_normalized;

        target_spec.ia_slot = primitive_assembler_input_slot;
        target_spec.element_stride = element_stride;
        target_spec.name = name;
        target_spec.name_index = name_index;
        target_spec.instancing_rate = instancing_data_rate;

        target_spec.instance = nullptr;

        TemplateArgumentIterator<
            VASpecificationLoopBody,
            VATargetSpecification::arg_pack0,
            VATargetSpecification::value_arg_pack0,
            VATargetSpecification::value_arg_pack1>::loop(&target_spec);

        return target_spec.instance;
    }

    std::string nodeMaskToString(uint32_t node_mask)
    {
        unsigned long node_index{ 0U };
        unsigned long accumulated_node_index{ 0U };
        std::string rv{"("};
        for (unsigned node_count = 0; _BitScanForward(&node_index, node_mask); node_mask >>= node_index + 1, accumulated_node_index += node_index + 1, ++node_count)
        {
            if (!node_count) rv += std::to_string(accumulated_node_index);
            else rv += "&" + std::to_string(accumulated_node_index);
        }
        rv += ")";
        return rv;
    }
}


class lexgine::core::dx::d3d12::D3D12PSOXMLParser::impl
{
public:
    impl(D3D12PSOXMLParser& parent) :
        m_parent{ parent }
        // m_deferred_compilation_exit_task_executed{ false },
        // m_deferred_shader_compilation_exit_task{ *this }
    {

    }

    tasks::HLSLCompilationTask* parseAndAddToCompilationCacheShader(pugi::xml_node& node, std::string const& pso_cache_name)
    {
        auto shader_node = node.find_child([](pugi::xml_node& n) -> bool
        {
            return std::strcmp(n.name(), "ComputeShader") == 0;
        });

        if (shader_node.empty())
        {
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "Error while parsing D3D12 PSO XML description from source \""
                + m_parent.m_source_xml + "\": obligatory attribute \"ComputeShader\" is not found");
        }

        pugi::char_t const* shader_source_location = shader_node.child_value();
        pugi::char_t const* shader_entry_point_name = shader_node.attribute("entry").as_string();
        ShaderModel sm = extractAttribute<attribute_type::shader_model>(shader_node.attribute("model"), ShaderModel::model_50);

        task_caches::HLSLFileTranslationUnit hlsl_translation_unit{ m_parent.m_globals,
            pso_cache_name + "(" + shader_source_location + ")__CS", shader_source_location };

        return m_parent.m_hlsl_compilation_task_cache.findOrCreateTask(hlsl_translation_unit, sm, 
            ShaderType::compute, shader_entry_point_name);
    }

    tasks::HLSLCompilationTask* parseAndAddToCompilationCacheShader(pugi::xml_node& node, 
        char const* p_stage_name, bool is_obligatory_shader_stage, 
        std::string const& pso_cache_name)
    {
        // parse and attempt to compile vertex shader
        auto shader_node = node.find_child([p_stage_name](pugi::xml_node& n) -> bool
        {
            return strcmp(n.name(), p_stage_name) == 0;
        });

        if (shader_node.empty())
        {
            if(is_obligatory_shader_stage)
            {
                LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "Error while parsing D3D12 PSO XML description from source \""
                    + m_parent.m_source_xml + "\": obligatory attribute " + p_stage_name + " was not found");
            }
            return nullptr;
        }

        pugi::char_t const* shader_source_location = shader_node.child_value();

        ShaderType shader_type;
        char* compilation_task_suffix = "";
        if (strcmp(p_stage_name, "VertexShader") == 0)
        {
            shader_type = ShaderType::vertex;
            compilation_task_suffix = "VS";
        }
        else if (strcmp(p_stage_name, "HullShader") == 0)
        {
            shader_type = ShaderType::hull;
            compilation_task_suffix = "HS";
        }
        else if (strcmp(p_stage_name, "DomainShader") == 0)
        {
            shader_type = ShaderType::domain;
            compilation_task_suffix = "DS";
        }
        else if (strcmp(p_stage_name, "GeometryShader") == 0)
        {
            shader_type = ShaderType::geometry;
            compilation_task_suffix = "GS";
        }
        else if (strcmp(p_stage_name, "PixelShader") == 0)
        {
            shader_type = ShaderType::pixel;
            compilation_task_suffix = "PS";
        }

        pugi::char_t const* shader_entry_point_name = shader_node.attribute("entry").as_string();
        ShaderModel sm = extractAttribute<attribute_type::shader_model>(shader_node.attribute("model"), ShaderModel::model_50);

        task_caches::HLSLFileTranslationUnit hlsl_translation_unit{ m_parent.m_globals,
            pso_cache_name + "(" + shader_source_location + ")__" + compilation_task_suffix, shader_source_location };

        return m_parent.m_hlsl_compilation_task_cache.findOrCreateTask(hlsl_translation_unit, sm, 
            shader_type, shader_entry_point_name);
    }

    tasks::RootSignatureCompilationTask* retrieveRootSignatureCompilationTask(pugi::xml_node& node)
    {
        std::string root_signature_name = node.attribute("root_signature_name").as_string("");
        if (root_signature_name.length())
        {
            auto& rs_cache = m_parent.m_root_signature_compilation_task_cache.storage();
            tasks::RootSignatureCompilationTask& target_task_ref =
                *std::find_if(rs_cache.begin(), rs_cache.end(),
                    [&root_signature_name](auto const& e) {return root_signature_name + "__ROOTSIGNATURE" == e.getCacheName(); });
            return &target_task_ref;
        }
        else
        {
            // NOTE: in future, if root signature reference is not defined, create custom root signature compilation task
            // based on shader reflexion. But currently, just throw exception
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "Pipeline state does not define root signature "
                "reference. Root signature definition based on reflexion of shaders is currently not implemented");
        }
    }

    tasks::GraphicsPSOCompilationTask* parseGraphicsPSO(pugi::xml_node& node, uint32_t node_mask)
    { 
        GraphicsPSODescriptor currently_assembled_pso_descriptor{};

        // Get attributes of the graphics PSO
        std::string pso_cache_name = node.attribute("name").as_string(("GraphicsPSO_" + m_parent.getId().toString()).c_str()) + ("_nodes" + nodeMaskToString(node_mask));
        
        currently_assembled_pso_descriptor.node_mask = node_mask;

        currently_assembled_pso_descriptor.primitive_restart = extractAttribute<attribute_type::boolean>(node.attribute("primitive_restart"), false);
        currently_assembled_pso_descriptor.primitive_topology_type = extractAttribute<attribute_type::primitive_topology>(node.attribute("primitive_topology"), lexgine::core::PrimitiveTopologyType::triangle);
        currently_assembled_pso_descriptor.sample_mask = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("sample_mask"), 0xFFFFFFFF);
        currently_assembled_pso_descriptor.multi_sampling_format.count = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("sample_count"), 1);
        currently_assembled_pso_descriptor.multi_sampling_format.quality = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("sample_quality"), 1);

        // Retrieve shader stages
        tasks::HLSLCompilationTask* p_vs_compilation_task = parseAndAddToCompilationCacheShader(node, "VertexShader", true, pso_cache_name);
        tasks::HLSLCompilationTask* p_hs_compilation_task = parseAndAddToCompilationCacheShader(node, "HullShader", false, pso_cache_name);
        tasks::HLSLCompilationTask* p_ds_compilation_task = parseAndAddToCompilationCacheShader(node, "DomainShader", false, pso_cache_name);
        tasks::HLSLCompilationTask* p_gs_compilation_task = parseAndAddToCompilationCacheShader(node, "GeometryShader", false, pso_cache_name);
        tasks::HLSLCompilationTask* p_ps_compilation_task = parseAndAddToCompilationCacheShader(node, "PixelShader", true, pso_cache_name);

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
                    LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                        + ": stream output descriptor must define attribute \"strides\"");
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
                            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                                + ": attribute \"name\" must be defined by \"DeclarationEntry\"");
                        }

                        char const* processed_name = name == "NULL" ? nullptr : name.c_str();
                        uint32_t name_index = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("name_index"), 0);
                        
                        uint32_t start_component = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("start_component"), 0, &is_attribute_present);
                        if (!is_attribute_present)
                        {
                            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                                + ": attribute \"start_component\" must be defined by \"DeclaraionEntry\"");
                        
                        }
                        uint32_t component_count = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("component_count"), 4, &is_attribute_present);
                        if (!is_attribute_present)
                        {
                            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                                + ": attribute \"component_count\" must be defined by \"DeclaraionEntry\"");
                        }
                        
                        uint32_t slot = extractAttribute<attribute_type::unsigned_numeric>(node.attribute("slot"), 0, &is_attribute_present);
                        if (!is_attribute_present)
                        {
                            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                                + ": attribute \"start_component\" must be defined by \"DeclaraionEntry\"");
                        }

                        so_entry_descs.emplace_back(stream, name.c_str(), name_index, start_component, component_count, slot);
                    }
                }

                currently_assembled_pso_descriptor.stream_output = lexgine::core::StreamOutput{ so_entry_descs, buffer_strides, rasterized_stream };
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

                currently_assembled_pso_descriptor.blend_state = lexgine::core::BlendState{ alpha_to_coverage, independent_blending };
                
                for (auto& blend_desc_node : blend_state_node)
                {
                    if (std::strcmp(blend_desc_node.name(), "BlendDescriptor") == 0)
                    {
                        bool is_attribute_present{ false };

                        uint8_t render_target =
                            static_cast<uint8_t>(extractAttribute<attribute_type::unsigned_numeric>(blend_desc_node.attribute("render_target"), 0, &is_attribute_present));
                        if (!is_attribute_present)
                        {
                            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                                + ": BlendDescriptor must have attribute \"render_target\"");
                        }

                        bool enable_blending = extractAttribute<attribute_type::boolean>(blend_desc_node.attribute("enable_blending"), false);
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

                        currently_assembled_pso_descriptor.blend_state.render_target_blend_descriptor[render_target] = lexgine::core::BlendDescriptor{
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

                currently_assembled_pso_descriptor.rasterization_descriptor =
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


                lexgine::core::StencilBehavior stencil_test_behavior[2];
                memset(stencil_test_behavior, 0, sizeof(stencil_test_behavior));
                for (auto& stencil_test_behavior_node : depth_stencil_desc_node)
                {
                    if (std::strcmp(stencil_test_behavior_node.name(), "StencilTestBehavior") == 0)
                    {
                        bool was_successful{ false };
                        auto front_face = extractAttribute<attribute_type::face>(stencil_test_behavior_node.attribute("face"), lexgine::core::CullMode::front, &was_successful);
                        if (!was_successful)
                        {
                            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                                + ": StencilTestBehavior node must define attribute \"face\"");
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
                            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name
                                + ": StencilTestBehavior node must define attribute \"dsv_target_format\"");
                        }
                        currently_assembled_pso_descriptor.dsv_format = dsv_target_format;

                        stencil_test_behavior[front_face == lexgine::core::CullMode::front ? 0 : 1] =
                            lexgine::core::StencilBehavior{ operation_st_fail, operation_st_pass_dt_fail, operation_st_pass_dt_pass, comparison_function };
                    }
                }

                currently_assembled_pso_descriptor.depth_stencil_descriptor = lexgine::core::DepthStencilDescriptor{ enable_depth_test, allow_depth_writes,
                depth_test_comparison_function, enable_stencil_test, stencil_test_behavior[0], stencil_test_behavior[1] };
            }

            

        }

        // Read vertex input attributes
        {
            auto va_specification_node = node.find_child([](pugi::xml_node& n) -> bool
            {
                return std::strcmp(n.name(), "VertexAttributeSpecification") == 0;
            });

            std::unordered_set<uint32_t> slots{};
            for (auto& va : va_specification_node)
            {
                if (std::strcmp(va.name(), "VertexAttribute") == 0)
                {
                    bool was_successful{ false };
                    std::string name = extractAttribute<attribute_type::string>(va.attribute("name"), "", &was_successful);
                    if (!was_successful)
                    {
                        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                            + ": VertexAttribute node must define attribute \"name\"");
                    }

                    uint32_t index = extractAttribute<attribute_type::unsigned_numeric>(va.attribute("index"), 0, &was_successful);
                    if (!was_successful)
                    {
                        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                            + ": VertexAttribute node must define attribute \"index\"");
                    }


                    uint32_t size = extractAttribute<attribute_type::unsigned_numeric>(va.attribute("size"), 0, &was_successful);
                    if (!was_successful)
                    {
                        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                            + ": VertexAttribute node must define attribute \"size\"");
                    }

                    DataFormat type = extractAttribute<attribute_type::data_format>(va.attribute("type"), DataFormat::float32, &was_successful);
                    if (!was_successful)
                    {
                        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                            + ": VertexAttribute node must define attribute \"type\"");
                    }

                    uint32_t slot = extractAttribute<attribute_type::unsigned_numeric>(va.attribute("slot"), 0, &was_successful);
                    if(!was_successful)
                    {
                        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name
                            + ": VertexAttribute node must define attribute \"slot\"");
                    }

                    if (!slots.insert(slot).second)
                    {
                        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name
                            + ": slot \"" + std::to_string(slot) + "\" is already used by another vertex attribute");
                    }

                    uint32_t element_stride = extractAttribute<attribute_type::unsigned_numeric>(va.attribute("stride"), D3D12_APPEND_ALIGNED_ELEMENT, &was_successful);
                    bool normalized = extractAttribute<attribute_type::boolean>(va.attribute("normalized"), false);
                    uint32_t instancing_rate = extractAttribute<attribute_type::unsigned_numeric>(va.attribute("instancing_rate"), 0);

                    currently_assembled_pso_descriptor.vertex_attributes.push_back(createVertexAttributeSpecification(type, size, normalized, slot, element_stride, name.c_str(), index, instancing_rate));
                }
            }
        }

        // Parse render target descriptors
        {
            uint8_t rt_occupied_slots[8]; std::fill_n(rt_occupied_slots, 8, 8);
            uint8_t num_rt{ 0U };
            for (auto& child_node : node)
            {
                if (std::strcmp(child_node.name(), "RenderTarget") == 0)
                {
                    bool was_successful{ false };
                    uint32_t slot = extractAttribute<attribute_type::unsigned_numeric>(child_node.attribute("slot"), 0, &was_successful);
                    if (!was_successful)
                    {
                        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                            + ": RenderTarget node must define attribute \"slot\"");
                    }

                    for (uint8_t i = 0; i < num_rt; ++i)
                    {
                        if (rt_occupied_slots[i] == slot)
                        {
                            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                                + ": render target slot \"" + std::to_string(slot) + "\" is already occupied");
                        }
                    }

                    rt_occupied_slots[num_rt] = slot;
                    ++num_rt;

                    DXGI_FORMAT format = extractAttribute<attribute_type::render_target_format>(child_node.attribute("format"), DXGI_FORMAT_R32G32B32A32_FLOAT, &was_successful);
                    if (!was_successful)
                    {
                        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                            + ": RenderTarget node must define attribute \"format\"");
                    }

                    currently_assembled_pso_descriptor.rtv_formats[slot] = format;
                }
                currently_assembled_pso_descriptor.num_render_targets = num_rt;

                std::sort(rt_occupied_slots, rt_occupied_slots + 8);
                bool is_slot_usage_range_valid = true;
                for (uint8_t i = 0; i < num_rt; ++i)
                {
                    if (rt_occupied_slots[i] != i)
                    {
                        is_slot_usage_range_valid = false;
                        break;
                    }
                }
                if (!is_slot_usage_range_valid)
                {
                    LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(m_parent, "error parsing XML PSO source of graphics PSO " + pso_cache_name 
                        + ": invalid render target slot usage range, the slot range in use should be contiguous and start at 0");
                }
            }
        }

        // Define new PSO compilation task
        tasks::GraphicsPSOCompilationTask* new_graphics_pso_compilation_task{ nullptr };
        {
            uint64_t uid = misc::HashedString{ pso_cache_name }.hash();
            new_graphics_pso_compilation_task = m_parent.m_pso_compilation_task_cache.findOrCreateTask(
                m_parent.m_globals,
                currently_assembled_pso_descriptor, 
                pso_cache_name, 
                uid);
            
            new_graphics_pso_compilation_task->setVertexShaderCompilationTask(p_vs_compilation_task);
            if (p_hs_compilation_task) new_graphics_pso_compilation_task->setHullShaderCompilationTask(p_hs_compilation_task);
            if (p_ds_compilation_task) new_graphics_pso_compilation_task->setDomainShaderCompilationTask(p_ds_compilation_task);
            if (p_gs_compilation_task) new_graphics_pso_compilation_task->setGeometryShaderCompilationTask(p_gs_compilation_task);
            new_graphics_pso_compilation_task->setPixelShaderCompilationTask(p_ps_compilation_task);

            new_graphics_pso_compilation_task->setRootSignatureCompilationTask(retrieveRootSignatureCompilationTask(node));
        }

        return new_graphics_pso_compilation_task;
    }

    tasks::ComputePSOCompilationTask* parseComputePSO(pugi::xml_node& node, uint32_t node_mask)
    {
        ComputePSODescriptor currently_assembled_pso_descriptor{};
        
        std::string pso_cache_name = node.attribute("name").as_string(("ComputePSO_" + m_parent.getId().toString()).c_str()) + ("_node" + nodeMaskToString(node_mask));

        currently_assembled_pso_descriptor.node_mask = node_mask;

        tasks::HLSLCompilationTask* p_cs_compilation_task = parseAndAddToCompilationCacheShader(node, pso_cache_name);
        
        tasks::ComputePSOCompilationTask* new_pso_compilation_task{ nullptr };
        {
            uint64_t uid = misc::HashedString{ pso_cache_name }.hash();

            new_pso_compilation_task = m_parent.m_pso_compilation_task_cache.findOrCreateTask(
                m_parent.m_globals,
                currently_assembled_pso_descriptor, 
                pso_cache_name, 
                uid);
            new_pso_compilation_task->setComputeShaderCompilationTask(p_cs_compilation_task);
            new_pso_compilation_task->setRootSignatureCompilationTask(retrieveRootSignatureCompilationTask(node));
        }

        return new_pso_compilation_task;
    }


private:
    D3D12PSOXMLParser& m_parent;
    // std::atomic_bool m_deferred_compilation_exit_task_executed;

private:

    #if 0
    class DeferredPSOCompilationExitTask : public concurrency::SchedulableTask
    {
    public:

        DeferredPSOCompilationExitTask(impl& parent) :
            SchedulableTask{ "deferred_pso_compilation_task_graph_exit_op" },
            m_p_sink{ nullptr },
            m_parent{ parent }
        {

        }

        void setInput(concurrency::TaskSink* p_sink)
        {
            m_p_sink = p_sink;
        }

        bool execute_manually()
        {
            return doTask(0, 0);
        }


    private:

        concurrency::TaskSink* m_p_sink;
        impl& m_parent;
        
        
    private:
        
        bool doTask(uint8_t /* worker_id */, uint64_t /* user?data */) override
        {
            m_parent.m_deferred_compilation_exit_task_executed = true;
            return true;
        }
        
        concurrency::TaskType type() const override
        {
            return concurrency::TaskType::cpu;
        }
        
    }m_deferred_shader_compilation_exit_task;
        
        
    public:

        DeferredPSOCompilationExitTask* deferredShaderCompilationExitTask()
        {
            return &m_deferred_shader_compilation_exit_task;
        }

        bool isDeferredShaderCompilationExitTaskExecuted() const
        {
            return m_deferred_compilation_exit_task_executed.load(std::memory_order_acquire);
        }

    #endif
};


lexgine::core::dx::d3d12::D3D12PSOXMLParser::D3D12PSOXMLParser(core::Globals& globals, std::string const& xml_source, bool deferred_shader_compilation, uint32_t node_mask) :
    m_globals{ globals },
    m_root_signature_compilation_task_cache{ *globals.get<task_caches::RootSignatureCompilationTaskCache>() },
    m_hlsl_compilation_task_cache{ *globals.get<task_caches::HLSLCompilationTaskCache>() },
    m_pso_compilation_task_cache{ *globals.get<task_caches::PSOCompilationTaskCache>() },
    m_source_xml{ xml_source },
    m_impl{ new impl{*this} }
{
    pugi::xml_document xml_doc;
    pugi::xml_parse_result parse_result = xml_doc.load_string(xml_source.c_str());

    if (parse_result)
    {
        if (!node_mask) node_mask = 0x1;

        for (pugi::xml_node_iterator it : xml_doc)
        {
            if (std::strcmp(it->name(), "GraphicsPSO") == 0)
            {
                m_parsed_graphics_pso_compilation_tasks.push_back(m_impl->parseGraphicsPSO(*it, node_mask));
            }
            else if (std::strcmp(it->name(), "ComputePSO") == 0)
            {
                m_parsed_compute_pso_compilation_tasks.push_back(m_impl->parseComputePSO(*it, node_mask));
            }
            else
            {
                LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, std::string{ "Unknown attribute \"" }
                    +it->name() + "\" encountered while parsing D3D12 PSO XML description from file \""
                    + xml_source + "\"");
            }
        }
    }
    else
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
            std::string{ "Unable to parse XML source\n" }
            + "Reason: " + parse_result.description() + "\n"
            + "Location: " + std::string(xml_source.c_str() + parse_result.offset,
                std::min<size_t>(xml_source.size() - parse_result.offset, 80ull))
        );
    }



    core::GlobalSettings const& global_settings = *m_globals.get<core::GlobalSettings>();
    if (global_settings.isDeferredPSOCompilationOn())
    {
        std::set<concurrency::TaskGraphRootNode*> root_tasks{};

        {
            for (tasks::GraphicsPSOCompilationTask* t : m_parsed_graphics_pso_compilation_tasks)
            {
                concurrency::TaskGraphRootNode* vs_task_ptr = ROOT_NODE_CAST(t->getVertexShaderCompilationTask());
                concurrency::TaskGraphRootNode* hs_task_ptr = ROOT_NODE_CAST(t->getHullShaderCompilationTask());
                concurrency::TaskGraphRootNode* ds_task_ptr = ROOT_NODE_CAST(t->getDomainShaderCompilationTask());
                concurrency::TaskGraphRootNode* gs_task_ptr = ROOT_NODE_CAST(t->getGeometryShaderCompilationTask());
                concurrency::TaskGraphRootNode* ps_task_ptr = ROOT_NODE_CAST(t->getPixelShaderCompilationTask());
                concurrency::TaskGraphRootNode* root_signature_task_ptr = ROOT_NODE_CAST(t->getRootSignatureCompilationTask());

                if (vs_task_ptr) root_tasks.insert(vs_task_ptr);
                if (hs_task_ptr) root_tasks.insert(hs_task_ptr);
                if (ds_task_ptr) root_tasks.insert(ds_task_ptr);
                if (gs_task_ptr) root_tasks.insert(gs_task_ptr);
                if (ps_task_ptr) root_tasks.insert(ps_task_ptr);
                if (root_signature_task_ptr) root_tasks.insert(root_signature_task_ptr);

                if (!global_settings.isDeferredShaderCompilationOn())
                {
                    vs_task_ptr->execute(0);
                    hs_task_ptr->execute(0);
                    ds_task_ptr->execute(0);
                    gs_task_ptr->execute(0);
                    ps_task_ptr->execute(0);
                }

                if (!global_settings.isDeferredRootSignatureCompilationOn())
                {
                    root_signature_task_ptr->execute(0);
                }
            }

            for (tasks::ComputePSOCompilationTask* t : m_parsed_compute_pso_compilation_tasks)
            {
                concurrency::TaskGraphRootNode* cs_task_ptr = ROOT_NODE_CAST(t->getComputeShaderCompilationTask());
                concurrency::TaskGraphRootNode* root_signature_task_ptr = ROOT_NODE_CAST(t->getRootSignatureCompilationTask());

                if (cs_task_ptr) root_tasks.insert(cs_task_ptr);
                if (root_signature_task_ptr) root_tasks.insert(root_signature_task_ptr);

                if (!global_settings.isDeferredShaderCompilationOn())
                    cs_task_ptr->execute(0);

                if (!global_settings.isDeferredRootSignatureCompilationOn())
                    root_signature_task_ptr->execute(0);
            }
        }

        #if 0
        for (auto& task : m_parsed_graphics_pso_compilation_tasks)
            task->addDependent(*m_impl->deferredShaderCompilationExitTask());

        for (auto& task : m_parsed_compute_pso_compilation_tasks)
            task->addDependent(*m_impl->deferredShaderCompilationExitTask());
        #endif


        concurrency::TaskGraph pso_compilation_task_graph{ std::set<concurrency::TaskGraphRootNode const*>{root_tasks.begin(), root_tasks.end()},
            global_settings.getNumberOfWorkers(), "deferred_pso_compilation_task_graph" };
        

        #ifdef LEXGINE_D3D12DEBUG
        pso_compilation_task_graph.createDotRepresentation("deferred_pso_compilation_task_graph__" + getId().toString() + ".gv");
        #endif

        std::vector<std::ostream*> worker_log_streams(global_settings.getNumberOfWorkers());
        auto& worker_file_logging_streams = globals.get<LoggingStreams>()->worker_logging_streams;
        std::transform(worker_file_logging_streams.begin(), worker_file_logging_streams.end(),
            worker_log_streams.begin(), [](std::ofstream& fs)->std::ostream* {return &fs; });

        concurrency::TaskSink task_sink{ pso_compilation_task_graph, worker_log_streams, "pso_compilation_task_sink_" + getId().toString() };
        task_sink.start();

        #if 0
        m_impl->deferredShaderCompilationExitTask()->setInput(&task_sink);
        #endif

        try
        {
            task_sink.submit(0);
        }
        catch (core::Exception& e)
        {
            std::string error_message = std::string{ "Unable to compile PSO blobs from XML description (" } +e.what() + "). See logs for further details";
            misc::Log::retrieve()->out(error_message, misc::LogMessageType::error);
            throw core::Exception{ *this, error_message };
        }
    }
    else
    {
        // since PSO compilation is dependent on root signature and shader compilation
        // immediate compilation of the PSOs requires the related shaders as well as the 
        // root signatures to be also compiled in immediate mode

        for (auto t : m_parsed_graphics_pso_compilation_tasks)
        {
            // compile the shaders
            t->getVertexShaderCompilationTask()->execute(0);
            t->getHullShaderCompilationTask()->execute(0);
            t->getDomainShaderCompilationTask()->execute(0);
            t->getGeometryShaderCompilationTask()->execute(0);
            t->getPixelShaderCompilationTask()->execute(0);

            // compile to root signature
            t->getRootSignatureCompilationTask()->execute(0);

            // compile the PSO itself
            t->execute(0);

            if (t->getErrorState())
            {
                std::string error_message = std::string{ "Unable to compile graphics PSO blob for task \"" } + t->getCacheName() + "\" (" 
                    + t->getErrorString() + "). See logs for further details.";
                misc::Log::retrieve()->out(error_message, misc::LogMessageType::error);
                throw core::Exception{ *this, error_message };
            }
        }

        for (auto t : m_parsed_compute_pso_compilation_tasks)
        {
            t->getComputeShaderCompilationTask()->execute(0);
            t->getRootSignatureCompilationTask()->execute(0);
            t->execute(0);

            if (t->getErrorState())
            {
                std::string error_message = std::string{ "Unable to compile compute PSO blob for task \"" } + t->getCacheName() + "\" (" 
                    + t->getErrorString() + "). See logs for further details.";
                misc::Log::retrieve()->out(error_message, misc::LogMessageType::error);
                throw core::Exception{ *this, error_message };
            }
        }

        // m_impl->deferredShaderCompilationExitTask()->execute_manually();
    }
}

D3D12PSOXMLParser::~D3D12PSOXMLParser() = default;

std::vector<tasks::GraphicsPSOCompilationTask*> const& D3D12PSOXMLParser::graphicsPSOCompilationTasks() const
{
    return m_parsed_graphics_pso_compilation_tasks;
}

std::vector<tasks::ComputePSOCompilationTask*> const& D3D12PSOXMLParser::computePSOCompilationTasks() const
{
    return m_parsed_compute_pso_compilation_tasks;
}
