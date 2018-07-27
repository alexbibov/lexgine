#ifndef LEXGINE_CORE_DX_DXGI_HW_ADAPTER_ENUMERATOR_H

#include <dxgi1_5.h>
#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <iterator>
#include <vector>
#include <list>

#include "hw_output_enumerator.h"
#include "../../../osinteraction/windows/window.h"
#include "swap_chain.h"

namespace lexgine { namespace core { namespace dx { namespace dxgi {


using namespace Microsoft::WRL;

//Direct3D 12 feature levels
enum class D3D12FeatureLevel : int
{
    _11_0 = D3D_FEATURE_LEVEL_11_0,
    _11_1 = D3D_FEATURE_LEVEL_11_1,
    _12_0 = D3D_FEATURE_LEVEL_12_0,
    _12_1 = D3D_FEATURE_LEVEL_12_1
};


//! Tiny wrapper over DXGI adapter interface
class HwAdapter final : public NamedEntity<class_names::HwAdapter>
{
    friend class HwAdapterEnumerator;   // friendship is not good design in general, but here it is just fine, since adapters must only be created by their enumerators
public:
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
        LUID luid;    //!< Local unique identifier of the adapter
    };

    struct Properties   //! Properties of DXGI adapter
    {
        D3D12FeatureLevel d3d12_feature_level;  //!< maximal feature level offered by Direct3D 12 and supported by the adapter
        uint32_t num_nodes;    //!< number of physical video adapters in the adapter link represented by this HwAdapter object
        MemoryDesc const* local;   //!< the fastest memory budget available to each physical adapter in the hardware link (i.e. the physical video memory on non-UMA devices)
        MemoryDesc const* non_local;   //!< non-local memory budget of each physical adapter in the hardware link (might be slower than the local budget and is the only one available to UMA devices)
        Description details;    //!< detailed description of the adapter
    };

    enum class MemoryBudget : int
    {
        local = DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
        non_local = DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL
    };

    //! Sends the OS notification requesting to reserve given @param amount_in_bytes from the video memory for the rendering application. THROWS.
    void reserveVideoMemory(uint32_t node_index, MemoryBudget budget_type, uint64_t amount_in_bytes) const;

    //! Returns properties structure describing this adapter
    Properties getProperties() const;

    //! Returns enumerator object that allows to iterate through hardware outputs of the adapter
    HwOutputEnumerator getOutputEnumerator() const;

    //! Creates swap chain for this adapter (or adapter link) and associates it with the given window
    SwapChain createSwapChain(osinteraction::windows::Window const& window, SwapChainDescriptor const& desc) const;

    //! Constructs wrapper around DXGI adapter. This is normally done only by HwAdapterEnumerator
    HwAdapter(ComPtr<IDXGIFactory4> const& adapter_factory, ComPtr<IDXGIAdapter3> const& adapter);

private:
    ComPtr<IDXGIAdapter3> m_dxgi_adapter;	//!< DXGI adapter
    ComPtr<IDXGIFactory4> m_dxgi_adapter_factory;    //!< DXGI factory, which has been used in order to create this adapter
    HwOutputEnumerator m_output_enumerator; //!< bi-directional iterator enumerating output devices attached to this adapter
    Properties m_properties;    //!< describes properties of the adapter

    class details;     //!< class encapsulating adapter details
    std::shared_ptr<details> m_p_details;    //!< pointer to adapter details buffer

    std::unique_ptr<d3d12::Device> m_device;    //!< d3d device associated with the adapter
};


//! Implements enumeration of hardware adapters supporting D3D12
class HwAdapterEnumerator final : public std::iterator<std::bidirectional_iterator_tag, HwAdapter>,
    public NamedEntity<class_names::HwAdapterEnumerator>
{
public:
    using const_iterator = std::list<HwAdapter>::const_iterator;

    HwAdapter const& operator*() const;
    HwAdapter const* operator->() const;

    HwAdapterEnumerator& operator++();
    HwAdapterEnumerator operator++(int);

    bool operator==(HwAdapterEnumerator const& other) const;
    bool operator!=(HwAdapterEnumerator const& other) const;

    HwAdapterEnumerator();
    HwAdapterEnumerator(HwAdapterEnumerator const& other) = default;

    HwAdapterEnumerator& operator = (HwAdapterEnumerator const& other) = default;

    HwAdapterEnumerator& operator--();
    HwAdapterEnumerator operator--(int);


    const_iterator begin() const;
    const_iterator end() const;

    const_iterator cbegin() const;
    const_iterator cend() const;


    void refresh();	//! refreshes the list of available hardware adapters with support of D3D12. THROWS.
    bool isRefreshNeeded() const;	//! returns true if the list of adapters has to be refreshed
    HwAdapter const& getWARPAdapter() const;   //! retrieves the WARP adapter


private:
    ComPtr<IDXGIFactory4> m_dxgi_factory4;
    std::list<HwAdapter> m_adapter_list;
    const_iterator m_adapter_iterator;
    uint32_t m_iterated_index;  //!< zero-based index of the currently iterated item
};

}}}}

#define LEXGINE_CORE_DX_DXGI_HW_ADAPTER_ENUMERATOR_H
#endif
