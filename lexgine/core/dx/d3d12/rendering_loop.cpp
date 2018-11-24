#include "rendering_loop.h"
#include "command_list.h"

#include <cassert>

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;


RenderingLoopTarget::RenderingLoopTarget(std::vector<Resource> const& target_resourcers,
    std::vector<ResourceState> const& target_resources_initial_states):
    m_target_resources{ target_resourcers }
{
    assert(target_resourcers.size() == target_resources_initial_states.size());


}

void RenderingLoopTarget::switchToRenderAccessState(CommandList const& command_list) const
{
    
}

void RenderingLoopTarget::switchToInitialState(CommandList const& command_list) const
{
}



RenderingLoop::RenderingLoop(Globals const& globals,
    std::shared_ptr<RenderingLoopTarget> const& rendering_loop_target_ptr):
    m_globals{ globals },
    m_rendering_tasks{ globals }
{
}

void RenderingLoop::draw()
{
}

