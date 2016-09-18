#include "blend_descriptor.h"

using namespace lexgine::core;

BlendDescriptor::BlendDescriptor():
    m_enable{ false },
    m_logic_op_enable{ false },
    m_color_write_mask{ ColorWriteMask{} | ColorWriteMask::enum_type::red | ColorWriteMask::enum_type::green | ColorWriteMask::enum_type::blue | ColorWriteMask::enum_type::alpha }
{

}

BlendDescriptor::BlendDescriptor(BlendFactor source_color_blend_factor, BlendFactor source_alpha_blend_factor,
    BlendFactor destination_color_blend_factor, BlendFactor destination_alpha_blend_factor,
    BlendOperation color_blend_operation, BlendOperation alpha_blend_operation,
    bool enable_logic_operations_on_destination, BlendLogicalOperation logical_operation_on_destination, ColorWriteMask const & color_write_mask):
    m_enable{ true },
    m_logic_op_enable{ enable_logic_operations_on_destination },
    m_src_blend{ source_color_blend_factor },
    m_dst_blend{ destination_color_blend_factor },
    m_blend_op{ color_blend_operation },
    m_src_alpha_blend{ source_alpha_blend_factor },
    m_dst_alpha_blend{ destination_alpha_blend_factor },
    m_alpha_blend_op{ alpha_blend_operation },
    m_logical_operation{ logical_operation_on_destination },
    m_color_write_mask{ color_write_mask }
{

}

BlendDescriptor::BlendDescriptor(BlendLogicalOperation logical_operation_on_destination, ColorWriteMask const & color_write_mask):
    m_enable{ false },
    m_logic_op_enable{ true },
    m_logical_operation{ logical_operation_on_destination },
    m_color_write_mask{ color_write_mask }
{

}

bool BlendDescriptor::isEnabled() const
{
    return m_enable;
}

bool BlendDescriptor::isLogicalOperationEnabled() const
{
    return m_logic_op_enable;
}

std::pair<BlendFactor, BlendFactor> BlendDescriptor::getSourceBlendFactors() const
{
    return std::make_pair(m_src_blend, m_src_alpha_blend);
}

std::pair<BlendFactor, BlendFactor> BlendDescriptor::getDestinationBlendFactors() const
{
    return std::make_pair(m_dst_blend, m_dst_alpha_blend);
}

std::pair<BlendOperation, BlendOperation> BlendDescriptor::getBlendOperation() const
{
    return std::make_pair(m_blend_op, m_alpha_blend_op);
}

BlendLogicalOperation BlendDescriptor::getBlendLogicalOperation() const
{
    return m_logical_operation;
}

ColorWriteMask BlendDescriptor::getColorWriteMask() const
{
    return m_color_write_mask;
}

BlendState::BlendState(bool alpha_to_coverage_enable, bool independent_blend_enable):
    alphaToCoverageEnable{ alpha_to_coverage_enable },
    independentBlendEnable{ independent_blend_enable }
{

}
