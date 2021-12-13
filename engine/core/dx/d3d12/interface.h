#ifndef LEXGINE_CORE_DX_D3D12_INTERFACE_H
#define LEXGINE_CORE_DX_D3D12_INTERFACE_H

#include "preprocessor_tokens.h"

namespace lexgine::core::dx::d3d12 {

struct LEXGINE_CPP_API GpuBasedValidationSettings
{
    bool enableGpuBasedValidation{ false };
    bool enableSynchronizedCommandQueueValidation{ false };
    bool disableResourceStateChecks{ false };
};

}

#endif