#include "debug_interface.h"

using namespace lexgine::core::dx::d3d12;

DebugInterface* DebugInterface::m_p_myself = nullptr;

DebugInterface const* DebugInterface::retrieve()
{
    if (!m_p_myself)
    {
        m_p_myself = new DebugInterface();
    }

    return m_p_myself;
}

void DebugInterface::shutdown()
{
    if (m_p_myself)
    {
        delete m_p_myself;
        m_p_myself = nullptr;
    }
}


DebugInterface::DebugInterface()
{
#ifdef D3D12DEBUG
    LEXGINE_ERROR_LOG(logger(), D3D12GetDebugInterface(IID_PPV_ARGS(&m_d3d12_debug)),
        std::bind(&DebugInterface::raiseError, this, std::placeholders::_1), S_OK);
    m_d3d12_debug->EnableDebugLayer();
#endif
}