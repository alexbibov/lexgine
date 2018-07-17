#ifndef LEXGINE_CORE_DX_D3D12_DEBUG_INTERFACE_H

#include <d3d12.h>
#include <wrl.h>

#include "../../entity.h"
#include "../../class_names.h"

using namespace Microsoft::WRL;

namespace lexgine {namespace core {namespace dx {namespace d3d12 {

//! Implements debug features for Direct3D 12. The proper usage assumes that application should have only
//! one instance of this class maintained in the application's main thread.
//! In addition, every API provided by this class has to perform nothing when the code is compiled with
//! LEXGINE_D3D12DEBUG switch off. DebugInterface::retrieve() on the other hand DOES instantiate the object even
//! when LEXGINE_D3D12DEBUG is off, but all the APIs provided on the instance level do nothing
class DebugInterface final : public NamedEntity<class_names::D3D12DebugInterface>
{
public:
    static DebugInterface const* retrieve();    //! creates debug interface for Direct3D 12
    static void shutdown();    //! destroys debug interface

private:
    //! This constructor THROWS when compiled in debug mode
    DebugInterface();
    DebugInterface(DebugInterface const& other) = delete;
    DebugInterface(DebugInterface&& other) = default;
    DebugInterface& operator=(DebugInterface const& other) = delete;
    DebugInterface& operator=(DebugInterface&& other) = delete;

    ComPtr<ID3D12Debug> m_d3d12_debug;  //!< Direct3D 12 debug layer interface
    static DebugInterface* m_p_myself;    //!< pointer to the instance of this class
};

}}}}

#define LEXGINE_CORE_DX_D3D12_DEBUG_INTERFACE_H
#endif
