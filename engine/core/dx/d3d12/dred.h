#ifndef LEXGINE_CORE_DX_D3D12_DRED_H
#define LEXGINE_CORE_DX_D3D12_DRED_H

#include <string>

#include "lexgine_core_dx_d3d12_fwd.h"


namespace lexgine::core::dx::d3d12 {

/*! returns a human-readable report of the device removed extended data recorded for @param device: the command lists
 the GPU had not finished, with the operations around the last one it completed, and the page fault, if any, with
 the allocations covering the faulting address. Returns an empty string when the device does not record the data
*/
std::string describeDeviceRemoval(Device const& device);

}

#endif
