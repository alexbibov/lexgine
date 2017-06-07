#include "depth_stencil_descriptor.h"

using namespace lexgine::core;


DepthStencilDescriptor::DepthStencilDescriptor(bool depth_test_enable, bool depth_writes, ComparisonFunction depth_test_predicate,
    bool stencil_test_enable, StencilBehavior front_face, StencilBehavior back_face, uint8_t stencil_read_mask, uint8_t stencil_write_mask):
    m_dt_enable{ depth_test_enable },
    m_depth_writes{ depth_writes },
    m_cmp_fun{ depth_test_predicate },
    m_st_enable{ stencil_test_enable },
    m_stencil_read_mask{ stencil_read_mask },
    m_stencil_write_mask{ stencil_write_mask },
    m_front_face{ front_face },
    m_back_face{ back_face }
{

}

bool DepthStencilDescriptor::isDepthTestEnabled() const
{
    return m_dt_enable;
}

bool DepthStencilDescriptor::isDepthUpdateAllowed() const
{
    return m_depth_writes;
}

ComparisonFunction DepthStencilDescriptor::depthTestPredicate() const
{
    return m_cmp_fun;
}

bool DepthStencilDescriptor::isStencilTestEnabled() const
{
    return m_st_enable;
}

uint8_t DepthStencilDescriptor::stencilReadMask() const
{
    return m_stencil_read_mask;
}

uint8_t DepthStencilDescriptor::stencilWriteMask() const
{
    return m_stencil_write_mask;
}

StencilBehavior DepthStencilDescriptor::stencilBehaviorForFrontFacingTriangles() const
{
    return m_front_face;
}

StencilBehavior DepthStencilDescriptor::stencilBehaviorForBackFacingTriangles() const
{
    return m_back_face;
}
