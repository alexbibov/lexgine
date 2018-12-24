#ifndef LEXGINE_CORE_DX_DXGI_HW_ADAPTER_ENUMERATOR_H
#define LEXGINE_CORE_DX_DXGI_HW_ADAPTER_ENUMERATOR_H

#include <d3d12.h>

#include <memory>
#include <iterator>
#include <vector>
#include <list>

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/osinteraction/windows/window.h"

#include "hw_output_enumerator.h"
#include "swap_chain.h"

namespace lexgine::core::dx::dxgi {

using namespace Microsoft::WRL;

//Direct3D 12 feature levels
enum class D3D12FeatureLevel : int
{
    _11_0 = D3D_FEATURE_LEVEL_11_0,
    _11_1 = D3D_FEATURE_LEVEL_11_1,
    _12_0 = D3D_FEATURE_LEVEL_12_0,
    _12_1 = D3D_FEATURE_LEVEL_12_1
};

template<typename T> class HwAdapterAttorney;

class HwAdapterEnumerator;

//! Tiny wrapper over DXGI adapter interface
class HwAdapter final : public NamedEntity<class_names::DXGI_HwAdapter>
{
    friend class HwAdapterAttorney<HwAdapterEnumerator>;

public:
    enum class DxgiGraphicsPreemptionGranularity
    {
        dma_buffer = DXGI_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY,
        primitive = DXGI_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY,
        triangle = DXGI_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY,
        pixel = DXGI_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY,
        instruction = DXGI_GRAPHICS_PREEMPTION_INSTRUCTION_BOUNDARY
    };

    enum class DxgiComputePreemptionGranularity
    {
        dma_buffer = DXGI_COMPUTE_PREEMPTION_DMA_BUFFER_BOUNDARY, 
        dispatch = DXGI_COMPUTE_PREEMPTION_DISPATCH_BOUNDARY,
        thread_group = DXGI_COMPUTE_PREEMPTION_THREAD_GROUP_BOUNDARY,
        thread = DXGI_COMPUTE_PREEMPTION_THREAD_BOUNDARY,
        instruction = DXGI_COMPUTE_PREEMPTION_INSTRUCTION_BOUNDARY
    };

    struct MemoryDesc   //! Description of adapter's memory budget (in bytes)
    {
        uint64_t total; //!< OS-provided total amount of video memory available to the adapter
        uint64_t available; //!< Video memory available for reservation by the application on this adapter
        uint64_t current_usage;   //!< Factual current usage of the video memory on this adapter by the application
        uint64_t current_reservation;   //!< Amount of video memory currently reserved by the application on this adapter
    };

    struct Description  //! Description of DXGI adapter
    {
        std::wstring name;    //!< Short string description of adapter
        uint32_t vendor_id;    //!< The PCI ID of hardware vendor
        uint32_t device_id;    //!< The PCI ID of hardware device
        uint32_t sub_system_id;    //!< The PCI ID of the sub system
        uint32_t revision;    //!< The PCI ID of the revision number of the adapter
        size_t dedicated_video_memory;    //!< amount of dedicated video memory represented in bytes
        size_t dedicated_system_memory;    //!< amount of dedicated system memory, in bytes, that is not shared with CPU
        size_t shared_system_memory;    //!< maximum amount of system memory, in bytes, that can be consumed by the adapter during operation
        LUID luid;    //!< Local unique identifier of the adapter
        DxgiGraphicsPreemptionGranularity graphics_preemption_granularity;    //!< describes granularity, at which the graphics adapter can be preempted from its current graphics task
        DxgiComputePreemptionGranularity compute_preemption_granularity;    //!< describes granularity, at which the graphics adapter can be preempted from its current compute task
    };

    struct Properties   //! Properties of DXGI adapter
    {
        D3D12FeatureLevel d3d12_feature_level;  //!< maximal feature level offered by Direct3D 12 and supported by the adapter
        uint32_t num_nodes;    //!< number of physical video adapters in the adapter link represented by this HwAdapter object
        std::vector<MemoryDesc> local;   //!< the fastest memory budget available to each physical adapter in the hardware link (i.e. the physical video memory on non-UMA devices)
        std::vector<MemoryDesc> non_local;   //!< non-local memory budget of each physical adapter in the hardware link (might be slower than the local budget and is the only one available to UMA devices)
        Description details;    //!< detailed description of the adapter
    };

    enum class MemoryBudget : int
    {
        local = DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
        non_local = DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL
    };

public:
    HwAdapter(HwAdapter const&) = delete;
    HwAdapter(HwAdapter&&) = default;

    ~HwAdapter();

public:
    //! Sends the OS notification requesting to reserve given amount_in_bytes from the video memory for the rendering application.
    void reserveVideoMemory(uint32_t node_index, MemoryBudget budget_type, uint64_t amount_in_bytes) const;

    //! Returns properties structure describing this adapter
    Properties getProperties() const;

    //! Returns enumerator object that allows to iterate through hardware outputs of the adapter
    HwOutputEnumerator const& getOutputEnumerator() const;

    //! Creates swap chain for this adapter (or adapter link) and associates it with the given window
    SwapChain createSwapChain(osinteraction::windows::Window const& window, SwapChainDescriptor const& desc,
        SwapChainAdvancedParameters const& advanced_parameters) const;

    d3d12::Device& device() const;

private:
    //! Constructs wrapper around DXGI adapter. This is normally done only by HwAdapterEnumerator
    HwAdapter(GlobalSettings const& global_settings,
        ComPtr<IDXGIFactory6> const& adapter_factory,
        ComPtr<IDXGIAdapter4> const& adapter, bool enable_debug_mode);

private:
    GlobalSettings const& m_global_settings;
    ComPtr<IDXGIAdapter4> m_dxgi_adapter;	//!< DXGI adapter
    ComPtr<IDXGIFactory6> m_dxgi_adapter_factory;    //!< DXGI factory, which has been used in order to create this adapter

    Properties m_properties;    //!< describes properties of the adapter

    class impl;
    std::unique_ptr<impl> m_impl;    //!< various properties of the adapter encapsulated in the details class
    
    std::unique_ptr<d3d12::Device> m_device;    //!< d3d12 device associated with the adapter
};

template<> class HwAdapterAttorney<HwAdapterEnumerator>
{
    friend class HwAdapterEnumerator;

private:
    static HwAdapter makeHwAdapter(GlobalSettings const& global_settings,
        ComPtr<IDXGIFactory6> const& adapter_factory,
        ComPtr<IDXGIAdapter4> const& adapter, bool enable_debug_mode)
    {
        return HwAdapter{ global_settings, adapter_factory, adapter, enable_debug_mode };
    }
};


//! Implements enumeration of hardware adapters supporting D3D12
class HwAdapterEnumerator final : public NamedEntity<class_names::DXGI_HwAdapterEnumerator>
{
public:
    using adapter_list_type = std::list<HwAdapter>;
    using iterator = adapter_list_type::iterator;
    using const_iterator = adapter_list_type::const_iterator;

    enum class DxgiGpuPreference
    {
        unspecified = DXGI_GPU_PREFERENCE_UNSPECIFIED,
        minimum_powert = DXGI_GPU_PREFERENCE_MINIMUM_POWER,
        high_performance = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE
    };

public:
    HwAdapterEnumerator(GlobalSettings const& global_settings,
        bool enable_debug_mode = false, 
        DxgiGpuPreference enumeration_preference = DxgiGpuPreference::high_performance);

    HwAdapterEnumerator(HwAdapterEnumerator const&) = delete;
    HwAdapterEnumerator(HwAdapterEnumerator&&) = delete;

public:
    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    const_iterator cbegin() const;
    const_iterator cend() const;


    void refresh(DxgiGpuPreference enumeration_preference = DxgiGpuPreference::high_performance);	//! refreshes the list of available hardware adapters with support of D3D12. THROWS.
    bool isRefreshNeeded() const;	//! returns true if the list of adapters has to be refreshed
    HwAdapter& getWARPAdapter();   //! retrieves the WARP adapter
    HwAdapter const& getWARPAdapter() const;    //! retrieves the WARP adapter

    uint32_t getAdapterCount() const;    //! retrieves total number of adapters installed in the host system including the WARP adapter


private:
    GlobalSettings const& m_global_settings;
    bool m_enable_debug_mode;
    ComPtr<IDXGIFactory6> m_dxgi_factory6;
    adapter_list_type m_adapter_list;
};

}

#endif
