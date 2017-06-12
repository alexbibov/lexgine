#ifndef LEXGINE_CORE_DEPTH_STENCIL_DESCRIPTOR_H

#include <cstdint>

namespace lexgine {namespace core {

//! Predicate operations determining when certain test based on comparison passes
enum class ComparisonFunction
{
    never,    //!< never pass the comparison
    less,    //!< passes if source data is less then destination data
    equal,    //!< passes if source data is equal to destination data
    less_or_equal,    //!< passes if source data is less than or equal to the destination data
    greater,    //!< passes if source data is greater than destination data
    not_equal,    //!< passes if source data is not equal to the destination data
    greater_equal,    //!< passes if source data is greater than or equal to the destination data
    always    //!< depth test always passes
};

//! Describes operations that can be performed on stencil buffer
enum class StencilOperation
{
    keep,    //!< keep existing data
    zero,    //!< set stencil data to zero
    replace,    //!< replace stencil data with reference value
    increment_and_saturate,    //!< increment currently stored stencil value by one and clamp the result
    decrement_and_saturate,    //!< decrement currently stored stencil value by one and clamp the result
    invert,    //!< invert stencil data (inverse the bit order)
    increment,    //!< increment current stencil value
    decrement    //!< decrement current stencil value
};

//! Describes behavior of the stencil buffer depending on results of depth-stencil test
struct StencilBehavior
{
    StencilOperation st_fail;    //!< operation performed when stencil test fails
    StencilOperation st_pass_dt_fail;    //!< operation performed when stencil test passes and depth test fails
    StencilOperation st_pass_dt_pass;    //!< operation performed when both stencil and depth tests are passed
    ComparisonFunction cmp_fun;    //!< comparison function determining when stencil test should pass
};


//! OS- and graphics API- agnostic encapsulation of depth-stencil state settings
//! The descriptor is intentionally immutable to enforce usage with less overhead
class DepthStencilDescriptor
{
public:
    DepthStencilDescriptor(bool depth_test_enable = true, bool depth_writes = true, ComparisonFunction depth_test_predicate = ComparisonFunction::less,
        bool stencil_test_enable = false,
        StencilBehavior const& front_face = StencilBehavior{ StencilOperation::keep, StencilOperation::keep, StencilOperation::keep, ComparisonFunction::always },
        StencilBehavior const& back_face = StencilBehavior{ StencilOperation::keep, StencilOperation::keep, StencilOperation::keep, ComparisonFunction::always },
        uint8_t stencil_read_mask = 0xFF, uint8_t stencil_write_mask = 0xFF);

    bool isDepthTestEnabled() const;
    bool isDepthUpdateAllowed() const;
    ComparisonFunction depthTestPredicate() const;
    bool isStencilTestEnabled() const;
    uint8_t stencilReadMask() const;
    uint8_t stencilWriteMask() const;
    StencilBehavior stencilBehaviorForFrontFacingTriangles() const;
    StencilBehavior stencilBehaviorForBackFacingTriangles() const;


private:
    bool m_dt_enable;    //!< 'true' if the depth test should be enabled
    bool m_depth_writes;      //!< when 'false' the writes to depth buffer are disabled
    ComparisonFunction m_cmp_fun;    //!< comparison function affecting when the test should be passed
    bool m_st_enable;    //!< when 'true' the stencil test is enabled
    uint8_t m_stencil_read_mask;    //!< identifies the portion of depth-stencil buffer from which to read the stencil data (this only has effect when depth and stencil are packed together)
    uint8_t m_stencil_write_mask;   //!< identifies the portion of depth-stencil buffer to which to write the stencil data (this only has effect when depth and stencil are packed together)
    StencilBehavior m_front_face;   //!< stencil buffer behavior applied to the front-facing triangles
    StencilBehavior m_back_face;    //!< stencil buffer behavior applied to the back-facing triangles
};

}}

#define LEXGINE_CORE_DEPTH_STENCIL_DESCRIPTOR_H
#endif
