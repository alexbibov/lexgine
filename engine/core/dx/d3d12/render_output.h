#ifndef LEXGINE_CORE_DX_D3D12_RENDER_OUTPUT_H
#define LEXGINE_CORE_DX_D3D12_RENDER_OUTPUT_H

#include <cstdint>
#include <memory>
#include <vector>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/dxgi/lexgine_core_dx_dxgi_fwd.h"
#include "engine/core/math/vector_types.h"

#include "resource.h"
#include "rendering_target.h"


namespace lexgine::core::dx::d3d12 {

enum class SwapChainDepthBufferFormat
{
    d32float_s8x24_uint = DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
    d32float = DXGI_FORMAT_D32_FLOAT,
    d24unorm_s8uint = DXGI_FORMAT_D24_UNORM_S8_UINT,
    d16unorm = DXGI_FORMAT_D16_UNORM
};


/*! Owns the surfaces a swap chain is presented from: the back buffers, the depth buffer shared by them and
 the rendering target describing each back buffer. Resize requests are recorded and applied on demand rather
 than when they arrive, so no device work is performed from within the window procedure.
*/
class RenderOutput final
{
public:
    RenderOutput(Globals& globals, dxgi::SwapChain& swap_chain, SwapChainDepthBufferFormat depth_buffer_format);

    RenderOutput(RenderOutput const&) = delete;
    RenderOutput(RenderOutput&& other);
    RenderOutput& operator=(RenderOutput const&) = delete;
    RenderOutput& operator=(RenderOutput&&) = delete;

    dxgi::SwapChain& swapChain() const { return m_swap_chain; }    //! returns the swap chain presenting this output

    DXGI_FORMAT depthBufferFormat() const { return m_depth_buffer_native_format; }    //! returns the native format of the depth buffer

    bool buffersAcquired() const { return m_buffers_acquired; }    //! returns 'true' if the surfaces are ready to be rendered into

    //! records a request to resize the output; the surfaces are recreated by the next call of applyPendingResize()
    void requestResize(uint32_t new_width, uint32_t new_height);

    /*! recreates the surfaces if a resize was requested and the output has a client area to render into.
     Returns 'true' if the surfaces have been recreated, which invalidates the dimensions reported previously.
     The frame progress tracker is used to await the GPU work still referencing the old surfaces.
    */
    bool applyPendingResize(FrameProgressTracker const& frame_progress_tracker);

    void acquireBuffers(uint32_t width, uint32_t height);    //! creates the surfaces for the given client area size

    //! destroys the surfaces once the GPU work referencing them has retired
    void releaseBuffers(FrameProgressTracker const& frame_progress_tracker);

    RenderingTarget& currentTarget();    //! returns the rendering target describing the back buffer to be presented next

    //! hands the back buffer to the presentation engine and reports how that went
    dxgi::PresentResult present() const;

    //! returns 'true' while the output is fully obscured and the presentation engine idles
    bool isOccluded() const;

private:
    Globals& m_globals;
    dxgi::SwapChain& m_swap_chain;

    bool m_buffers_acquired = false;
    bool m_pending_resize = false;
    uint32_t m_pending_width = 0U;
    uint32_t m_pending_height = 0U;

    std::vector<Resource> m_color_buffers;
    std::unique_ptr<CommittedResource> m_depth_buffer;

    DXGI_FORMAT m_depth_buffer_native_format;
    ResourceOptimizedClearValue m_depth_optimized_clear_value;

    std::vector<RenderingTarget> m_targets;
};

}

#endif
