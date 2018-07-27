#include "hw_adapter_enumerator.h"
#include "lexgine/core/misc/log.h"
#include "lexgine/core/exception.h"

using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx::dxgi;
using namespace lexgine::core::misc;


HwAdapterEnumerator::HwAdapterEnumerator()
{
    refresh();
}

HwAdapter const& HwAdapterEnumerator::operator*() const
{
    return *m_adapter_iterator;
}

HwAdapter const* HwAdapterEnumerator::operator->() const
{
    return &(*m_adapter_iterator);
}

HwAdapterEnumerator& HwAdapterEnumerator::operator++()
{
    ++m_iterated_index;
    ++m_adapter_iterator;

    return *this;
}

HwAdapterEnumerator HwAdapterEnumerator::operator++(int)
{
    HwAdapterEnumerator temp{ *this };
    ++m_iterated_index;
    ++m_adapter_iterator;

    return temp;
}

bool HwAdapterEnumerator::operator==(HwAdapterEnumerator const& other) const
{
    // If any of the enumerators being compared require refresh of the data
    // it means that at least one of them does not contain up-to-the-date information regarding the host
    // system and their comparison does not make any sense
    if (isRefreshNeeded() || other.isRefreshNeeded()) return false;

    return m_iterated_index == other.m_iterated_index;
}

bool HwAdapterEnumerator::operator!=(HwAdapterEnumerator const& other) const
{
    return !this->operator==(other);
}

HwAdapterEnumerator& HwAdapterEnumerator::operator--()
{
    --m_iterated_index;
    --m_adapter_iterator;

    return *this;
}

HwAdapterEnumerator HwAdapterEnumerator::operator--(int)
{
    HwAdapterEnumerator temp{ *this };
    --m_iterated_index;
    --m_adapter_iterator;

    return temp;
}

HwAdapterEnumerator::const_iterator HwAdapterEnumerator::begin() const
{
    return m_adapter_list.begin();
}

HwAdapterEnumerator::const_iterator HwAdapterEnumerator::end() const
{
    return m_adapter_list.end();
}

HwAdapterEnumerator::const_iterator HwAdapterEnumerator::cbegin() const
{
    return m_adapter_list.cbegin();
}

HwAdapterEnumerator::const_iterator HwAdapterEnumerator::cend() const
{
    return m_adapter_list.cend();
}

void HwAdapterEnumerator::refresh()
{
    m_adapter_list.clear();

    ComPtr<IDXGIFactory2> dxgi_factory2;

    //Create DXGI factory
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgi_factory2)),
        S_OK
    );
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        dxgi_factory2->QueryInterface(IID_PPV_ARGS(&m_dxgi_factory4)),
        S_OK
    );
    //dxgi_factory2->Release();

    //Enumerate DXGI adapters and attempt to create DX12 device with minimal required feature level (i.e. dx11.0) with
    //every of them. If the call successes, add the corresponding adapter to the iteration list
    IDXGIAdapter* p_dxgi_adapter{ nullptr };
    IDXGIAdapter3* p_dxgi_adapter3{ nullptr };
    HRESULT hres = S_OK;
    UINT id = 0;
    while ((hres = m_dxgi_factory4->EnumAdapters(id, &p_dxgi_adapter)) != DXGI_ERROR_NOT_FOUND)
    {
        if (hres != S_OK)
        {
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(
                this,
                "error while enumerating adapters"
            );
        }

        LEXGINE_LOG_ERROR_IF_FAILED(
            this,
            p_dxgi_adapter->QueryInterface(__uuidof(IDXGIAdapter3), reinterpret_cast<void**>(&p_dxgi_adapter3)),
            S_OK
        );
        p_dxgi_adapter->Release();
        if(!p_dxgi_adapter3) continue;

        HRESULT res;
        if (p_dxgi_adapter3 
            && ((res = D3D12CreateDevice(p_dxgi_adapter3, static_cast<D3D_FEATURE_LEVEL>(D3D12FeatureLevel::_11_0), __uuidof(ID3D12Device), nullptr)) == S_OK || res == S_FALSE))
            m_adapter_list.emplace_back(m_dxgi_factory4, ComPtr<IDXGIAdapter3>{p_dxgi_adapter3});
        else
        {
            DXGI_ADAPTER_DESC2 desc2;
            p_dxgi_adapter3->GetDesc2(&desc2);
            std::string adapter_name = wstringToAsciiString(desc2.Description);

            LEXGINE_LOG_ERROR(
                this, 
                "unable to create Direct3D12 device for adapter \"" + adapter_name + "\""
            );
        }
        p_dxgi_adapter3->Release();

        ++id;
    }

    //Add WARP adapter to the end of the list (i.e. the WARP adapter is always enumerated and it is always the last adapter in the list)
    p_dxgi_adapter = nullptr;
    p_dxgi_adapter3 = nullptr;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_dxgi_factory4->EnumWarpAdapter(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&p_dxgi_adapter)),
        S_OK
    );

    if(p_dxgi_adapter)
    {
        LEXGINE_LOG_ERROR_IF_FAILED(
            this,
            p_dxgi_adapter->QueryInterface(__uuidof(IDXGIAdapter3), reinterpret_cast<void**>(&p_dxgi_adapter3)),
            S_OK
        );
        m_adapter_list.emplace_back(m_dxgi_factory4, ComPtr<IDXGIAdapter3> { p_dxgi_adapter3 });
        if (p_dxgi_adapter3) p_dxgi_adapter3->Release();
        p_dxgi_adapter->Release();
    }
    
    m_adapter_iterator = m_adapter_list.begin();
    m_iterated_index = 0;
}

bool HwAdapterEnumerator::isRefreshNeeded() const
{
    return !m_dxgi_factory4->IsCurrent();
}

HwAdapter const& HwAdapterEnumerator::getWARPAdapter() const { return m_adapter_list.back(); }




class HwAdapter::details final
{
public:
    void refreshMemoryStatistics();    // refreshes memory budget statistics
    MemoryDesc const* getLocalMemoryDescription() const;     // returns pointer to the series of descriptors of the local memory budgets
    MemoryDesc const* getNonLocalMemoryDescription() const;    // returns pointer to the series of descriptors of the non-local memory budgets

    details();
    details(HwAdapter* p_outer);
    ~details();

private:
    HwAdapter* m_p_outer;    // main part of the implementation

    uint32_t m_node_count;    // number of nodes in the adapter link
    MemoryDesc* m_local_memory_desc;    // describes local memory budget (this is usually the faster memory available to the hardware; it can also be unavailable at all)
    MemoryDesc* m_non_local_memory_desc;    // describes non-local memory budget (this is usually the slower memory available to the hardware)
};


HwAdapter::details::details() :
    m_local_memory_desc{ nullptr },
    m_non_local_memory_desc{ nullptr }
{

}

HwAdapter::details::details(HwAdapter* p_outer):
    m_p_outer{ p_outer },
    m_node_count{ m_p_outer->m_device->native()->GetNodeCount() },
    m_local_memory_desc{ new MemoryDesc[m_node_count] },
    m_non_local_memory_desc{ new MemoryDesc[m_node_count] }
{

}

HwAdapter::details::~details()
{
    if (m_local_memory_desc)
        m_node_count > 1 ? delete[] m_local_memory_desc : delete m_local_memory_desc;

    if (m_non_local_memory_desc)
        m_node_count > 1 ? delete[] m_non_local_memory_desc : delete m_non_local_memory_desc;
}

HwAdapter::MemoryDesc const* HwAdapter::details::getLocalMemoryDescription() const
{
    return m_local_memory_desc;
}

HwAdapter::MemoryDesc const* HwAdapter::details::getNonLocalMemoryDescription() const
{
    return m_non_local_memory_desc;
}

void HwAdapter::details::refreshMemoryStatistics()
{
    for (uint32_t i = 0; i < m_p_outer->m_device->native()->GetNodeCount(); ++i)
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO vm_info_desc;

        LEXGINE_THROW_ERROR_IF_FAILED(
            m_p_outer,
            m_p_outer->m_dxgi_adapter->QueryVideoMemoryInfo(i, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &vm_info_desc),
            S_OK
        );
        m_local_memory_desc[i].total = vm_info_desc.Budget;
        m_local_memory_desc[i].available = vm_info_desc.AvailableForReservation;
        m_local_memory_desc[i].current_usage = vm_info_desc.CurrentUsage;
        m_local_memory_desc[i].current_reservation = vm_info_desc.CurrentReservation;

        LEXGINE_THROW_ERROR_IF_FAILED(
            m_p_outer,
            m_p_outer->m_dxgi_adapter->QueryVideoMemoryInfo(i, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &vm_info_desc),
            S_OK
        );
        m_non_local_memory_desc[i].total = vm_info_desc.Budget;
        m_non_local_memory_desc[i].available = vm_info_desc.AvailableForReservation;
        m_non_local_memory_desc[i].current_usage = vm_info_desc.CurrentUsage;
        m_non_local_memory_desc[i].current_reservation = vm_info_desc.CurrentReservation;
    }
}



HwAdapter::HwAdapter(ComPtr<IDXGIFactory4> const& adapter_factory, ComPtr<IDXGIAdapter3> const& adapter) :
    m_dxgi_adapter{ adapter },
    m_dxgi_adapter_factory{ adapter_factory },
    m_p_details{ nullptr }
{
    DXGI_ADAPTER_DESC2 desc;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        adapter->GetDesc2(&desc),
        S_OK
    );

    m_output_enumerator = HwOutputEnumerator{ adapter, desc.AdapterLuid };

    m_properties.details.name = desc.Description;
    m_properties.details.vendor_id = desc.VendorId;
    m_properties.details.device_id = desc.DeviceId;
    m_properties.details.sub_system_id = desc.SubSysId;
    m_properties.details.revision = desc.Revision;
    m_properties.details.luid.LowPart = desc.AdapterLuid.LowPart;
    m_properties.details.luid.HighPart = desc.AdapterLuid.HighPart;

    // attempt to create D3D12 device in order to determine feature level of the hardware and receive information regarding the memory budgets

    ComPtr<ID3D12Device> d3d12_device{ nullptr };
    for (auto feature_level : { std::make_pair(D3D12FeatureLevel::_12_1, "12.1"), std::make_pair(D3D12FeatureLevel::_12_0, "12.0"),
        std::make_pair(D3D12FeatureLevel::_11_1, "11.1"), std::make_pair(D3D12FeatureLevel::_11_0, "11.0") })
    {
        if (D3D12CreateDevice(m_dxgi_adapter.Get(), static_cast<D3D_FEATURE_LEVEL>(static_cast<int>(feature_level.first)),
            IID_PPV_ARGS(&d3d12_device)) == S_OK)
        {
            logger().out("Device " + std::string{ m_properties.details.name.begin(), m_properties.details.name.end() } +
                ": feature level " + feature_level.second + " detected", LogMessageType::information);
            m_p_details = std::shared_ptr<details>{ new details{ this } };
            m_properties.d3d12_feature_level = feature_level.first;
            m_properties.num_nodes = d3d12_device->GetNodeCount();
            m_properties.local = m_p_details->getLocalMemoryDescription();
            m_properties.non_local = m_p_details->getNonLocalMemoryDescription();

            setStringName(wstringToAsciiString(m_properties.details.name) + "_VendorID:" + std::to_string(m_properties.details.vendor_id)
                + "_DeviceID:" + std::to_string(m_properties.details.device_id) + "_SubSystemID:" + std::to_string(m_properties.details.sub_system_id)
                + "_Revision:" + std::to_string(m_properties.details.revision) + "_LUID:" + std::to_string(m_properties.details.luid.HighPart)
                + std::to_string(m_properties.details.luid.LowPart));

            break;
        }
    }


    if (d3d12_device)
    {
        m_device.reset(new d3d12::Device{ d3d12_device });
    }
    else
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "no device compatible with Direct3D 12 found");
        return;
    }
}


void HwAdapter::reserveVideoMemory(uint32_t node_index, MemoryBudget budget_type, uint64_t amount_in_bytes) const
{
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_dxgi_adapter->SetVideoMemoryReservation(node_index, static_cast<DXGI_MEMORY_SEGMENT_GROUP>(static_cast<int>(budget_type)), amount_in_bytes),
        S_OK
    );
}

HwAdapter::Properties HwAdapter::getProperties() const
{
    m_p_details->refreshMemoryStatistics();
    return m_properties;
}

HwOutputEnumerator HwAdapter::getOutputEnumerator() const
{
    return m_output_enumerator;
}

SwapChain HwAdapter::createSwapChain(osinteraction::windows::Window const& window, SwapChainDescriptor const& desc) const
{
    return SwapChain{ m_dxgi_adapter_factory, *m_device, window, desc };
}
