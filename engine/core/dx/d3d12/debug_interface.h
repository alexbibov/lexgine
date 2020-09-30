#ifndef LEXGINE_CORE_DX_D3D12_DEBUG_INTERFACE_H
#define LEXGINE_CORE_DX_D3D12_DEBUG_INTERFACE_H

#include <d3d12.h>
#include <wrl.h>

#include "engine/core/entity.h"
#include "engine/core/class_names.h"

using namespace Microsoft::WRL;

namespace lexgine::core::dx::d3d12 {

struct GpuBasedValidationSettings
{
    bool enableGpuBasedValidation{ false };
    bool enableSynchronizedCommandQueueValidation{ false };
    bool disableResourceStateChecks{ false };
};

/*! Implements debug features for Direct3D 12. The proper usage assumes that application should have only
 one instance of this class maintained in the application's main thread.
 In addition, every API provided by this class has to perform nothing when the code is compiled with
 LEXGINE_D3D12DEBUG switch off. DebugInterface::retrieve() on the other hand DOES instantiate the object even
 when LEXGINE_D3D12DEBUG is off, but all the APIs provided on the instance level do nothing
*/
class DebugInterface final : public NamedEntity<class_names::D3D12_DebugInterface>
{
public:
    static DebugInterface const* create(GpuBasedValidationSettings const& gpu_based_validation_settings);    //! creates debug interface for Direct3D 12
    static DebugInterface const* retrieve();
    static void shutdown();    //! destroys debug interface

    GpuBasedValidationSettings const& gpuValidationSettings() const { return m_gpu_validation_settings; }

private:
    DebugInterface(GpuBasedValidationSettings const& gpu_based_validation_settings);
    DebugInterface(DebugInterface const& other) = delete;
    DebugInterface(DebugInterface&& other) = default;
    DebugInterface& operator=(DebugInterface const& other) = delete;
    DebugInterface& operator=(DebugInterface&& other) = delete;


    ComPtr<ID3D12Debug1> m_d3d12_debug;  //!< Direct3D 12 debug layer interface
    GpuBasedValidationSettings m_gpu_validation_settings;    //!< GPU based validation settings employed by the debug layer
    static DebugInterface* m_p_myself;    //!< pointer to the instance of this class
};

}

#endif
