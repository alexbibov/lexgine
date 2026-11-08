#include "swap_chain_link.h"

#include "engine/core/concurrency/thread_affinity.h"
#include "engine/core/exception.h"
#include "engine/core/globals.h"
#include "engine/core/global_settings.h"
#include "engine/core/dx/dxgi/swap_chain.h"

#include "rendering_tasks.h"
#include "device.h"
#include "frame_progress_tracker.h"

using namespace lexgine::core;
using namespace lexgine::core::dx;
using namespace lexgine::core::dx::d3d12;


SwapChainLink::SwapChainLink(Globals& globals, dxgi::SwapChain& swap_chain_to_link,
    SwapChainDepthBufferFormat depth_buffer_format)
    : m_globals{ globals }
    , m_global_settings{ globals.globalSettings() }
    , m_linked_swap_chain{ swap_chain_to_link }
    , m_linked_rendering_tasks_ptr{ nullptr }
    , m_render_output{ globals, swap_chain_to_link, depth_buffer_format }
{

}

SwapChainLink::SwapChainLink(SwapChainLink&& other)
    : m_globals{ other.m_globals }
    , m_global_settings{ other.m_global_settings }
    , m_linked_swap_chain{ other.m_linked_swap_chain }
    , m_linked_rendering_tasks_ptr{ other.m_linked_rendering_tasks_ptr }
    , m_suspend_rendering{ other.m_suspend_rendering }
    , m_render_output{ std::move(other.m_render_output) }
    , m_frame_pacer{ other.m_frame_pacer }
{

}

std::shared_ptr<SwapChainLink> SwapChainLink::create(Globals& globals, dxgi::SwapChain& swap_chain_to_link, SwapChainDepthBufferFormat depth_buffer_format)
{
    return std::shared_ptr<SwapChainLink>{ new SwapChainLink{globals, swap_chain_to_link, depth_buffer_format} };
}

SwapChainLink::~SwapChainLink()
{
    // the thread driving the frames flushes the GPU work before it leaves its loop, and it is joined before
    // the link it drives is destroyed, so there is nothing left to await here
}

void SwapChainLink::linkRenderingTasks(RenderingTasks* p_rendering_loop_to_link)
{
    m_linked_rendering_tasks_ptr = p_rendering_loop_to_link;
    m_frame_pacer.emplace(p_rendering_loop_to_link->frameProgressTracker(),
        m_global_settings.getMaxFramesInFlight(), m_linked_swap_chain.backBufferCount());
    updateRenderingConfiguration();
}

FrameOutcome SwapChainLink::render()
{
    if (m_linked_swap_chain.isDeviceLost()) return FrameOutcome::device_lost;

    if (m_frame_pacer && m_render_output.applyPendingResize(m_frame_pacer->frameProgressTracker()))
    {
        updateRenderingConfiguration();
        m_suspend_rendering = false;
    }

    if (!m_render_output.buffersAcquired())
    {
        core::math::Vector2u swap_chain_dimensions = m_linked_swap_chain.getDimensions();
        m_render_output.acquireBuffers(swap_chain_dimensions.x, swap_chain_dimensions.y);
        updateRenderingConfiguration();
        m_suspend_rendering = false;
    }

    if (m_render_output.isOccluded())
    {
        // the presentation engine idles while the output is obscured: the test present is what reveals that
        // it has become visible again
        m_render_output.present();
        return m_render_output.isOccluded() ? FrameOutcome::occluded : FrameOutcome::rendered;
    }

    if (m_suspend_rendering || !m_linked_rendering_tasks_ptr) return FrameOutcome::suspended;

    RenderingTarget& target = m_render_output.currentTarget();
    m_frame_pacer->waitForFrameSlot();

    m_frame_pacer->markFrameBegin();
    try
    {
        m_linked_rendering_tasks_ptr->render(target, [this]()
            {
                m_render_output.present();
                return m_linked_swap_chain.presentStatistics();
            });
    }
    catch (Exception const&)
    {
        // a device removed while the frame was recorded fails the frame from whichever call noticed it first
        if (FAILED(m_globals.device().native()->GetDeviceRemovedReason())) return FrameOutcome::device_lost;
        throw;
    }
    m_frame_pacer->markFrameEnd();

    return m_render_output.isOccluded() ? FrameOutcome::occluded : FrameOutcome::rendered;
}

void SwapChainLink::flushRenderingTasks()
{
    if (m_linked_rendering_tasks_ptr) m_linked_rendering_tasks_ptr->flush();
}

void SwapChainLink::requestResize(uint32_t new_width, uint32_t new_height)
{
    concurrency::assertOnMasterWorkerThread();

    m_render_output.requestResize(new_width, new_height);
}

void SwapChainLink::suspendRendering()
{
    concurrency::assertOnMasterWorkerThread();

    m_suspend_rendering = true;
}

void SwapChainLink::updateRenderingConfiguration() const
{
    math::Vector2u swap_chain_dimensions = m_linked_swap_chain.getDimensions();
    Viewport viewport{ math::Vector2f{0.f, 0.f}, math::Vector2f{ static_cast<float>(swap_chain_dimensions.x),
        static_cast<float>(swap_chain_dimensions.y)},
    math::Vector2f{0, 1.f} };

    m_linked_rendering_tasks_ptr->defineRenderingConfiguration(
        RenderingConfiguration{
            viewport,
            m_linked_swap_chain.descriptor().format,
            m_render_output.depthBufferFormat()
        }
    );
}
