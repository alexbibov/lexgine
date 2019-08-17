#ifndef LEXGINE_CORE_DX_D3D12_H
#define LEXGINE_CORE_DX_D3D12_H

#include <cstdint>
#include <windows.h>
#include <pix3.h>

namespace lexgine::core::dx::d3d12 {

namespace pix_marker_colors {

uint32_t constexpr PixCPUJobMarkerColor = 0xFF3333FF;    // dark blue
uint32_t constexpr PixGPUGeneralJobColor = 0xFFCC00CC;    // purple
uint32_t constexpr PixGPUGraphicsJobMarkerColor = 0xFFFFFF33;    // yellow
uint32_t constexpr PixGPUComputeJobMarkerColor = 0xFFCC0000;    // dark red
uint32_t constexpr PixGPUCopyJobMarkerColor = 0xFF606060;    // gray

}

}

#endif
