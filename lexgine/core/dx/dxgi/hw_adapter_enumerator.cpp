#include "hw_adapter_enumerator.h"
#include "../../misc/log.h"

using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx::dxgi;
using namespace lexgine::core::misc;



HwAdapterEnumerator::HwAdapterEnumerator()
{
    refresh();
}


HwAdapter& HwAdapterEnumerator::operator*()
{
    return *m_adapter_iterator;
}

HwAdapter const& HwAdapterEnumerator::operator*() const
{
    return *m_adapter_iterator;
}

HwAdapterEnumerator::iterator& HwAdapterEnumerator::operator->()
{
    return m_adapter_iterator;
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

HwAdapterEnumerator::iterator HwAdapterEnumerator::begin()
{
    return m_adapter_list.begin();
}

HwAdapterEnumerator::iterator HwAdapterEnumerator::end()
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

    IDXGIFactory2* p_dxgi_factory2;

    //Create DXGI factory
    LEXGINE_ERROR_LOG(
        this,
        CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&p_dxgi_factory2)),
        S_OK
    );
    LEXGINE_ERROR_LOG(
        this,
        p_dxgi_factory2->QueryInterface(IID_PPV_ARGS(&m_dxgi_factory4)),
        S_OK
    );
    p_dxgi_factory2->Release();

    //Enumerate DXGI adapters and attempt to create DX12 device with minimal required feature level (i.e. dx11.0) with
    //every of them. If the call successes, add the corresponding adapter to the iteration list
    IDXGIAdapter* p_dxgi_adapter;
    IDXGIAdapter3* p_dxgi_adapter3;
    HRESULT hres = S_OK;
    UINT id = 0;
    while ((hres = m_dxgi_factory4->EnumAdapters(id, &p_dxgi_adapter)) != DXGI_ERROR_NOT_FOUND)
    {
        if (hres != S_OK)
        {
            char const* err_msg = "error while enumerating adapters";
            logger().out(err_msg);
            raiseError(err_msg);
            return;
        }

        LEXGINE_ERROR_LOG(
            this,
            p_dxgi_adapter->QueryInterface(__uuidof(IDXGIAdapter3), reinterpret_cast<void**>(&p_dxgi_adapter3)),
            S_OK
        );
        p_dxgi_adapter->Release();

        HRESULT res;
        if ((res = D3D12CreateDevice(p_dxgi_adapter3, static_cast<D3D_FEATURE_LEVEL>(D3D12FeatureLevel::_11_0), __uuidof(ID3D12Device), nullptr)) == S_OK || res == S_FALSE)
            m_adapter_list.emplace_back(m_dxgi_factory4, ComPtr<IDXGIAdapter3>{p_dxgi_adapter3});
        else
        {
            DXGI_ADAPTER_DESC2 desc2;
            p_dxgi_adapter3->GetDesc2(&desc2);

            char* adapter_name = new char[wcslen(desc2.Description)];
            for (size_t i = 0; i < wcslen(desc2.Description); ++i)
                adapter_name[i] = static_cast<char>(desc2.Description[i]);
            logger().out(std::string{ "unable to create Direct3D12 device for adapter \"" } + adapter_name + "\"");
            delete[] adapter_name;
        }
        p_dxgi_adapter3->Release();

        ++id;
    }

    //Add WARP adapter to the end of the list (i.e. the WARP adapter is always enumerated and it is always the last adapter in the list)
    LEXGINE_ERROR_LOG(
        this,
        m_dxgi_factory4->EnumWarpAdapter(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&p_dxgi_adapter)),
        S_OK
    );
    LEXGINE_ERROR_LOG(
        this,
        p_dxgi_adapter->QueryInterface(__uuidof(IDXGIAdapter3), reinterpret_cast<void**>(&p_dxgi_adapter3)),
        S_OK
    );
    m_adapter_list.emplace_back(m_dxgi_factory4, ComPtr<IDXGIAdapter3> {p_dxgi_adapter3});
    p_dxgi_adapter3->Release();

    m_adapter_iterator = m_adapter_list.begin();
    m_iterated_index = 0;
}

bool HwAdapterEnumerator::isRefreshNeeded() const
{
    return !m_dxgi_factory4->IsCurrent();
}

HwAdapter HwAdapterEnumerator::getWARPAdapter() const { return m_adapter_list.back(); }




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
    m_node_count{ m_p_outer->m_d3d12_device->GetNodeCount() },
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
    for (uint32_t i = 0; i < m_p_outer->m_d3d12_device->GetNodeCount(); ++i)
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO vm_info_desc;

        LEXGINE_ERROR_LOG(
            m_p_outer,
            m_p_outer->m_dxgi_adapter->QueryVideoMemoryInfo(i, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &vm_info_desc),
            S_OK
        );
        m_local_memory_desc[i].total = vm_info_desc.Budget;
        m_local_memory_desc[i].available = vm_info_desc.AvailableForReservation;
        m_local_memory_desc[i].current_usage = vm_info_desc.CurrentUsage;
        m_local_memory_desc[i].current_reservation = vm_info_desc.CurrentReservation;

        LEXGINE_ERROR_LOG(
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
    LEXGINE_ERROR_LOG(
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

    bool is_device_created = false;
    for (auto feature_level : { std::make_pair(D3D12FeatureLevel::_12_1, "12.1"), std::make_pair(D3D12FeatureLevel::_12_0, "12.0"),
        std::make_pair(D3D12FeatureLevel::_11_1, "11.1"), std::make_pair(D3D12FeatureLevel::_11_0, "11.0") })
    {
        if (D3D12CreateDevice(m_dxgi_adapter.Get(), static_cast<D3D_FEATURE_LEVEL>(static_cast<int>(feature_level.first)),
            IID_PPV_ARGS(&m_d3d12_device)) == S_OK)
        {
            logger().out("Device " + std::string{ m_properties.details.name.begin(), m_properties.details.name.end() } +
                ": feature level " + feature_level.second + " detected");
            m_p_details = std::shared_ptr<details>{ new details{ this } };
            m_properties.d3d12_feature_level = feature_level.first;
            m_properties.num_nodes = m_d3d12_device->GetNodeCount();
            m_properties.local = m_p_details->getLocalMemoryDescription();
            m_properties.non_local = m_p_details->getNonLocalMemoryDescription();

            setStringName(wstringToAsciiString(m_properties.details.name) + "_VendorID:" + std::to_string(m_properties.details.vendor_id)
                + "_DeviceID:" + std::to_string(m_properties.details.device_id) + "_SubSystemID:" + std::to_string(m_properties.details.sub_system_id)
                + "_Revision:" + std::to_string(m_properties.details.revision) + "_LUID:" + std::to_string(m_properties.details.luid.HighPart)
                + std::to_string(m_properties.details.luid.LowPart));

            is_device_created = true;

            break;
        }
    }


    if (!is_device_created)
    {
        raiseError("no device compatible with Direct3D 12 found");
        return;
    }
}


void HwAdapter::reserveVideoMemory(uint32_t node_index, MemoryBudget budget_type, uint64_t amount_in_bytes) const
{
    LEXGINE_ERROR_LOG(
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
    return SwapChain{ m_dxgi_adapter_factory, d3d12::Device{m_d3d12_device}, window, desc };
}
