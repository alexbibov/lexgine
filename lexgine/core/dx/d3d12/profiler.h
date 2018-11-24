#ifndef LEXGINE_CORE_DX_D3D12_PROFILER_H
#define LEXGINE_CORE_DX_D3D12_PROFILER_H

#include <d3d12.h>
#include "pix3.h"
#include <cstdint>

namespace lexgine::core::dx::d3d12 {

namespace pix_marker_colors {

extern uint32_t const PixCPUJobMarkerColor;    // dark blue
extern uint32_t const PixGPUGeneralJobColor;    // purple
extern uint32_t const PixGPUGraphicsJobMarkerColor;    // yellow
extern uint32_t const PixGPUComputeJobMarkerColor;    // dark red
extern uint32_t const PixGPUCopyJobMarkerColor;    // gray

}

}

#endif    // LEXGINE_CORE_DX_D3D12_PROFILER_H
