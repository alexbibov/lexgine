#ifndef LEXGINE_CORE_DX_D3D12_H
#define LEXGINE_CORE_DX_D3D12_H

#include <cstdint>
#include <windows.h>
#include <pix3.h>

namespace lexgine::core::dx::d3d12 {

namespace pix_marker_colors {

extern uint32_t const PixCPUJobMarkerColor;    // dark blue
extern uint32_t const PixGPUGeneralJobColor;    // purple
extern uint32_t const PixGPUGraphicsJobMarkerColor;    // yellow
extern uint32_t const PixGPUComputeJobMarkerColor;    // dark red
extern uint32_t const PixGPUCopyJobMarkerColor;    // gray

}

}

#endif
