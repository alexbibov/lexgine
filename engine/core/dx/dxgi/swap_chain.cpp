#include <sstream>

#include "engine/core/exception.h"
#include "engine/core/misc/log.h"
#include "engine/core/dx/d3d12/command_queue.h"

#include "swap_chain.h"


namespace {

uint32_t constexpr c_max_frame_latency = 16;

uint32_t reconcileFrameLatency(uint32_t max_frame_latency)
{
    if (max_frame_latency <= c_max_frame_latency) return max_frame_latency;

    if (lexgine::core::misc::Log* p_logger = lexgine::core::misc::Log::retrieve())
    {
        std::stringstream err{};
        err << "ERROR: the maximal frame latency (" << max_frame_latency << ") exceeds the limit of the presentation "
            "engine. The latency is reduced to " << c_max_frame_latency;
        p_logger->out(err.str(), lexgine::core::misc::LogMessageType::error);
    }

    return c_max_frame_latency;
}

std::chrono::steady_clock::time_point toSteadyClockTime(int64_t qpc_time)
{
    LARGE_INTEGER qpc_now{};
    QueryPerformanceCounter(&qpc_now);
    std::chrono::steady_clock::time_point const steady_now = std::chrono::steady_clock::now();

    LARGE_INTEGER qpc_frequency{};
    QueryPerformanceFrequency(&qpc_frequency);

    std::chrono::duration<double> const age{ static_cast<double>(qpc_now.QuadPart - qpc_time) / qpc_frequency.QuadPart };
    return steady_now - std::chrono::duration_cast<std::chrono::steady_clock::duration>(age);
}

}  // namespace


namespace lexgine::core::dx::dxgi {

namespace
{
    bool isDeviceLostResult(HRESULT hres)
    {
        return hres == DXGI_ERROR_DEVICE_REMOVED || hres == DXGI_ERROR_DEVICE_RESET;
    }
}  // namespace

d3d12::Device& lexgine::core::dx::dxgi::SwapChain::device() const
{
    return m_device;
}

d3d12::CommandQueue const& SwapChain::defaultCommandQueue() const
{
    return m_default_command_queue;
}

math::Vector2u SwapChain::getDimensions() const
{
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1;
    LEXGINE_THROW_ERROR_IF_FAILED(this,
        m_dxgi_swap_chain->GetDesc1(&swap_chain_desc1),
        S_OK);

    return math::Vector2u{ swap_chain_desc1.Width, swap_chain_desc1.Height };
}

dx::d3d12::Resource SwapChain::getBackBuffer(uint32_t buffer_index) const
{
    ComPtr<ID3D12Resource> backbuffer_interface;

    LEXGINE_THROW_ERROR_IF_FAILED(this,
        m_dxgi_swap_chain->GetBuffer(static_cast<UINT>(buffer_index), IID_PPV_ARGS(backbuffer_interface.GetAddressOf())),
        S_OK);

    return dx::d3d12::Resource{ backbuffer_interface };
}

uint32_t SwapChain::getCurrentBackBufferIndex() const
{
    return static_cast<uint32_t>(m_dxgi_swap_chain->GetCurrentBackBufferIndex());
}

PresentResult SwapChain::present() const
{
    if (m_device_lost) return PresentResult::device_lost;

    if (!m_swapChainIsIdle)
    {
        HRESULT rv = m_dxgi_swap_chain->Present(
            m_descriptor.enable_vsync ? 1 : 0,
            m_descriptor.enable_vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING
        );

        if (rv == DXGI_STATUS_OCCLUDED)
        {
            m_swapChainIsIdle = true;
            return PresentResult::occluded;
        }

        if (rv != S_OK) return reportPresentFailure(rv);

        return PresentResult::ok;
    }

    HRESULT rv = m_dxgi_swap_chain->Present(m_descriptor.enable_vsync ? 1 : 0, DXGI_PRESENT_TEST);

    if (rv == S_OK)
    {
        m_swapChainIsIdle = false;
        return PresentResult::ok;
    }

    if (isDeviceLostResult(rv)) return reportPresentFailure(rv);

    return PresentResult::occluded;
}

FrameLatencyWait SwapChain::waitForFrameLatency(std::chrono::milliseconds timeout, HANDLE interrupt_event) const
{
    if (!m_frame_latency_waitable_object) return FrameLatencyWait::ready;

    HANDLE const handles[] = { interrupt_event, m_frame_latency_waitable_object.get() };
    switch (WaitForMultipleObjectsEx(2, handles, FALSE, static_cast<DWORD>(timeout.count()), TRUE))
    {
    case WAIT_OBJECT_0:
        return FrameLatencyWait::interrupted;

    case WAIT_OBJECT_0 + 1:
        return FrameLatencyWait::ready;

    default:
        return FrameLatencyWait::timed_out;
    }
}

PresentStatistics SwapChain::presentStatistics() const
{
    PresentStatistics rv{};

    UINT last_present_count{};
    if (FAILED(m_dxgi_swap_chain->GetLastPresentCount(&last_present_count))) return rv;
    rv.present_count = static_cast<uint32_t>(last_present_count);

    DXGI_FRAME_STATISTICS frame_statistics{};
    if (SUCCEEDED(m_dxgi_swap_chain->GetFrameStatistics(&frame_statistics)) && frame_statistics.PresentCount)
    {
        rv.last_displayed = DisplayedPresent{ .present_count = static_cast<uint32_t>(frame_statistics.PresentCount),
            .display_time = toSteadyClockTime(frame_statistics.SyncQPCTime.QuadPart) };
    }

    return rv;
}

PresentResult SwapChain::reportPresentFailure(HRESULT hres) const
{
    bool const device_lost = isDeviceLostResult(hres);
    if (device_lost) m_device_lost = true;

    if (misc::Log* p_logger = misc::Log::retrieve())
    {
        std::stringstream err{};
        err << "ERROR: " << (device_lost ? "the device backing the swap chain was lost" : "Present failed")
            << " (0x" << std::hex << hres << ")";
        p_logger->out(err.str(), misc::LogMessageType::error);
    }

    return device_lost ? PresentResult::device_lost : PresentResult::failed;
}

uint32_t SwapChain::backBufferCount() const
{
    return m_descriptor.back_buffer_count;
}

SwapChainDescriptor SwapChain::descriptor() const
{
    return m_descriptor;
}

void SwapChain::resizeBuffers(math::Vector2u const& new_dimensions)
{
    // TODO: AFR support

    std::vector<UINT> node_masks(m_descriptor.back_buffer_count, 0x1);
    std::vector<IUnknown*> queues(m_descriptor.back_buffer_count, m_default_command_queue.native().Get());

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_dxgi_swap_chain->ResizeBuffers1(0,
            static_cast<UINT>(new_dimensions.x), static_cast<UINT>(new_dimensions.y),
            DXGI_FORMAT_UNKNOWN, m_flags, node_masks.data(), queues.data()),
        S_OK
    );
}

SwapChain::SwapChain(ComPtr<IDXGIFactory6> const& dxgi_factory,
    d3d12::Device& device,
    d3d12::CommandQueue const& default_command_queue,
    HWND hwnd,
    math::Vector2u const& client_area_size,
    SwapChainDescriptor const& desc)
    : m_dxgi_factory{ dxgi_factory }
    , m_device{ device }
    , m_hwnd{ hwnd }
    , m_default_command_queue{ default_command_queue }
    , m_descriptor{ desc }
    , m_flags{ DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING }
{
    m_descriptor.max_frame_latency = reconcileFrameLatency(desc.max_frame_latency);
    if (m_descriptor.max_frame_latency) m_flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1{};

    swap_chain_desc1.Width = static_cast<UINT>(client_area_size.x);
    swap_chain_desc1.Height = static_cast<UINT>(client_area_size.y);
    swap_chain_desc1.Format = desc.format;
    swap_chain_desc1.Stereo = desc.stereo;
    swap_chain_desc1.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };
    swap_chain_desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc1.BufferCount = static_cast<UINT>(desc.back_buffer_count);
    swap_chain_desc1.Scaling = static_cast<DXGI_SCALING>(desc.scaling);
    swap_chain_desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc1.Flags = m_flags;

    IDXGISwapChain1* p_swap_chain1 = nullptr;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_dxgi_factory->CreateSwapChainForHwnd(m_default_command_queue.native().Get(),
            m_hwnd, &swap_chain_desc1, nullptr, nullptr, &p_swap_chain1),
        S_OK
    );

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        p_swap_chain1->QueryInterface(IID_PPV_ARGS(&m_dxgi_swap_chain)),
        S_OK
    );
    p_swap_chain1->Release();

    if (m_descriptor.max_frame_latency)
    {
        LEXGINE_THROW_ERROR_IF_FAILED(
            this,
            m_dxgi_swap_chain->SetMaximumFrameLatency(static_cast<UINT>(m_descriptor.max_frame_latency)),
            S_OK
        );
        m_frame_latency_waitable_object.reset(m_dxgi_swap_chain->GetFrameLatencyWaitableObject());
    }

    // the application owns the fullscreen toggle; DXGI's default Alt+Enter handling would fight it
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_dxgi_factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER),
        S_OK
    );
}

}
