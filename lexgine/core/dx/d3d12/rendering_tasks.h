#ifndef LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H
#define LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine_core_dx_d3d12_fwd.h"


namespace lexgine::core::dx::d3d12 {

class RenderingTasks 
{
public:
    RenderingTasks(Globals const& globals);

private:
    Globals const& m_globals;

};

}

#endif // !LEXGINE_CORE_DX_D3D12_RENDERING_TASKS_H

