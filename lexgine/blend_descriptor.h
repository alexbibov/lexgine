#ifndef LEXGINE_CORE_BLEND_DESCRIPTOR

#include <cstdint>
#include <utility>

#include "flags.h"

namespace lexgine {namespace core {

//! Encapsulates blending factors
enum class BlendFactor: uint8_t
{
    zero,
    one,
    source_color,
    one_minus_source_color,
    source_alpha,
    one_minus_source_alpha,
    destination_alpha,
    one_minus_destination_alpha,
    destination_color,
    one_minus_destination_color,
    source_alpha_saturation,
    custom_constant,
    one_minus_custom_constant,
    source1_color,
    one_minus_source1_color,
    source1_alpha,
    one_minus_source1_alpha
};

//! Encapsulated blending operations
enum class BlendOperation : uint8_t
{
    add,    //!< source1 + destination
    subtract,    //!< source - destination
    reverse_subtract,    //!< destination - source
    min,    //!< min(source1, destination)
    max,    //!< max(source2, destination)
};

//! Encapsulates logical operations that can be performed on render target
enum class BlendLogicalOperation: uint8_t
{
    clear,
    set,
    copy,
    copy_inverted,
    no_operation,
    invert,
    and,
    nand, // logical "not and"
    or,
    nor, // logical "not or"
    xor,
    equiv, // logical "equality" (true iff both operands have same logical value)
    and_then_reverse,   // performs logical "and" and then reverses the result (i.e. computes 1 - result)
    and_then_invert,    // performs logical "and" and then inverts the bit order of the result
    or_then_reverse,   // performs logical "or" and then reverses the result (i.e. computes 1 - result)
    or_then_invert    // performs logical "or" and then inverts the bit order of the result
};


namespace __tag {
enum class tagColorWriteEnable {
    red = 1, green = 2, blue = 4, alpha = 8
};
}


//! Color write mask (same at least for Direct3D12 and Vulkan. Check if same applies to Metal)
using ColorWriteMask = ::lexgine::core::misc::Flags<__tag::tagColorWriteEnable>;


//! Encapsulates description of the blending stage. This class is API and OS-agnostic.
//! Blend descriptors are intentionally immutable
class BlendDescriptor final
{
public:
    BlendDescriptor(); //! default blend state with blending switched off

    //! Initializes blend state descriptor, which is by default enabled
    BlendDescriptor(BlendFactor source_color_blend_factor, BlendFactor source_alpha_blend_factor,
        BlendFactor destination_color_blend_factor, BlendFactor destination_alpha_blend_factor,
        BlendOperation color_blend_operation, BlendOperation alpha_blend_operation,
        bool enable_logic_operations_on_destination = false, BlendLogicalOperation logical_operation_on_destination = BlendLogicalOperation::clear,
        ColorWriteMask const& color_write_mask = ColorWriteMask{}
        | ColorWriteMask::enum_type::red | ColorWriteMask::enum_type::green | ColorWriteMask::enum_type::blue | ColorWriteMask::enum_type::alpha);

    //! Initialized blend state descriptor with disabled blending and enabled logical operations
    BlendDescriptor(BlendLogicalOperation logical_operation_on_destination,
        ColorWriteMask const& color_write_mask = ColorWriteMask{}
        | ColorWriteMask::enum_type::red | ColorWriteMask::enum_type::green | ColorWriteMask::enum_type::blue | ColorWriteMask::enum_type::alpha);

    bool isEnabled() const;    //! returns 'true' if the blend descriptor describes state with enabled blending
    bool isLogicalOperationEnabled() const;    //! returns 'true' if logical operation on target is enabled during the blending stage

    std::pair<BlendFactor, BlendFactor> getSourceBlendFactors() const;    //! returns RGB and Alpha source blend factors packed into std::pair
    std::pair<BlendFactor, BlendFactor> getDestinationBlendFactors() const;    //! returns RGB and Alpha destination blend factors packed into std::pair

    std::pair<BlendOperation, BlendOperation> getBlendOperation() const;    //! returns RGB and Alpha blend operations packed into std::pair

    BlendLogicalOperation getBlendLogicalOperation() const;    //! returns blend logical operation encapsulated by descriptor

    ColorWriteMask getColorWriteMask() const;    //! returns color write mask

private:
    bool m_enable;    //!< 'true' if blending is enabled
    bool m_logic_op_enable;    //!< 'true' if logic operations are enabled for the blending stage
    BlendFactor m_src_blend;   //!< blend factor applied to the source color
    BlendFactor m_dst_blend;   //!< blend factor applied to the destination color
    BlendOperation m_blend_op;    //!< blending operation: op(RGBs*m_src_blend, RGBd*m_dst_blend)
    BlendFactor m_src_alpha_blend;    //!< blend factor applied to the source alpha
    BlendFactor m_dst_alpha_blend;    //!< blend factor applied to the destination alpha
    BlendOperation m_alpha_blend_op;    //!< blending operation applied to the alpha-channel: op(As*m_src_alpha_blend, Ad*m_dst_alpha_blend)
    BlendLogicalOperation m_logical_operation;    //!< logical operation applied to the render target
    ColorWriteMask m_color_write_mask;    //!< describes which color channels are to be updated by the blending operation
};


//! Describes blending state
struct BlendState
{
    bool alphaToCoverageEnable;
    bool independentBlendEnable;
    BlendDescriptor render_target_blend_descriptor[8];    //!< blend descriptor for each enabled render target (if independent blending is disabled, only the first one is used)

    BlendState() = default;
    BlendState(bool alpha_to_coverage_enable, bool independent_blend_enable);
};


}}

#define LEXGINE_CORE_BLEND_DESCRIPTOR
#endif
