#ifndef LEXGINE_CORE_RENDERING_FRAME_CONTEXT_H
#define LEXGINE_CORE_RENDERING_FRAME_CONTEXT_H

#include <cstdint>

#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"


namespace lexgine::core {

/*! State describing a single frame being recorded. The contexts are kept in a ring, so the state of a frame
 stays readable by the threads recording it while the state of the following frames is being prepared.
*/
struct FrameContext
{
    uint64_t frame_index;    //!< index of the frame this context describes
    dx::d3d12::RenderingTarget* p_rendering_target;    //!< target the frame is rendered into
};

}

#endif
