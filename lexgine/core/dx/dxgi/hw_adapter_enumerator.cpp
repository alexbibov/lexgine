#include "hw_adapter_enumerator.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/misc/log.h"
#include "lexgine/core/misc/misc.h"
#include "lexgine/core/dx/d3d12/device.h"

using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx::dxgi;
using namespace lexgine::core::misc;


HwAdapterEnumerator::HwAdapterEnumerator(GlobalSettings const& global_settings,
    bool enable_debug_mode, DxgiGpuPreference enumeration_preference) :
    m_global_settings{ global_settings },
    m_enable_debug_mode{ enable_debug_mode }
{
    ComPtr<IDXGIFactory2> dxgi_factory2;

    //Create DXGI factory

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        CreateDXGIFactory2(enable_debug_mode ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&dxgi_factory2)),
        S_OK
    );

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        dxgi_factory2->QueryInterface(IID_PPV_ARGS(&m_dxgi_factory6)),
        S_OK
    );

    refresh(enumeration_preference);
}

HwAdapterEnumerator::iterator HwAdapterEnumerator::begin()
{
    return m_adapter_list.begin();
}

HwAdapterEnumerator::iterator HwAdapterEnumerator::end()
{
    return m_adapter_list.end();
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

void HwAdapterEnumerator::refresh(DxgiGpuPreference enumeration_preference)
{
    m_adapter_list.clear();

    // Enumerate DXGI adapters and attempt to create DX12 device with minimal required feature level (i.e. dx11.0) for
    // each of them. If the call succeeds, then add the corresponding adapter to the iteration list
    IDXGIAdapter* dxgi_adapter{ nullptr };

    HRESULT hres = S_OK;
    UINT id = 0;
    while ((hres = m_dxgi_factory6->EnumAdapterByGpuPreference(id,
        static_cast<DXGI_GPU_PREFERENCE>(enumeration_preference), __uuidof(IDXGIAdapter),
        reinterpret_cast<void**>(&dxgi_adapter))) != DXGI_ERROR_NOT_FOUND)
    {
        if (hres != S_OK)
        {
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "error while enumerating adapters");
        }

        ComPtr<IDXGIAdapter4> dxgi_adapter4{ nullptr };
        LEXGINE_LOG_ERROR_IF_FAILED(
            this,
            dxgi_adapter->QueryInterface(IID_PPV_ARGS(&dxgi_adapter4)),
            S_OK
        );
        dxgi_adapter->Release();
        if (!dxgi_adapter4) continue;

        HRESULT res;
        if ((res = D3D12CreateDevice(dxgi_adapter4.Get(), static_cast<D3D_FEATURE_LEVEL>(D3D12FeatureLevel::_11_0), __uuidof(ID3D12Device), nullptr)) == S_OK || res == S_FALSE)
            m_adapter_list.emplace_back(HwAdapterAttorney<HwAdapterEnumerator>::makeHwAdapter(m_global_settings, m_dxgi_factory6, dxgi_adapter4, m_enable_debug_mode));
        else
        {
            DXGI_ADAPTER_DESC3 desc;
            dxgi_adapter4->GetDesc3(&desc);
            std::string adapter_name = wstringToAsciiString(desc.Description);

            LEXGINE_LOG_ERROR(this,
                "unable to create Direct3D12 device for adapter \"" + adapter_name + "\". "
                "The possible reason is that the adapter does not support Direct3D 12");
        }

        ++id;
    }
}

bool HwAdapterEnumerator::isRefreshNeeded() const
{
    return !m_dxgi_factory6->IsCurrent();
}

HwAdapter& HwAdapterEnumerator::getWARPAdapter() { return m_adapter_list.back(); }
HwAdapter const& HwAdapterEnumerator::getWARPAdapter() const { return m_adapter_list.back(); }

uint32_t HwAdapterEnumerator::getAdapterCount() const
{
    return static_cast<uint32_t>(m_adapter_list.size());
}




class HwAdapter::impl final
{
public:
    impl(HwAdapter& enclosing, ComPtr<IDXGIAdapter4> const& adapter, LUID luid, uint32_t num_nodes) :
        m_enclosing{ enclosing },
        m_output_enumerator{ adapter, luid },
        m_num_nodes{ num_nodes },
        m_local_memory_desc(num_nodes),
        m_non_local_memory_desc(num_nodes)
    {

    }

    HwOutputEnumerator const& getOutputEnumeratorForThisAdapter() const
    {
        return m_output_enumerator;
    }

    void refreshMemoryStatistics()
    {
        for (uint32_t i = 0; i < m_num_nodes; ++i)
        {
            DXGI_QUERY_VIDEO_MEMORY_INFO vm_info_desc;

            LEXGINE_THROW_ERROR_IF_FAILED(
                m_enclosing,
                m_enclosing.m_dxgi_adapter->QueryVideoMemoryInfo(i, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &vm_info_desc),
                S_OK
            );
            m_local_memory_desc[i].total = vm_info_desc.Budget;
            m_local_memory_desc[i].available = vm_info_desc.AvailableForReservation;
            m_local_memory_desc[i].current_usage = vm_info_desc.CurrentUsage;
            m_local_memory_desc[i].current_reservation = vm_info_desc.CurrentReservation;

            LEXGINE_THROW_ERROR_IF_FAILED(
                m_enclosing,
                m_enclosing.m_dxgi_adapter->QueryVideoMemoryInfo(i, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &vm_info_desc),
                S_OK
            );
            m_non_local_memory_desc[i].total = vm_info_desc.Budget;
            m_non_local_memory_desc[i].available = vm_info_desc.AvailableForReservation;
            m_non_local_memory_desc[i].current_usage = vm_info_desc.CurrentUsage;
            m_non_local_memory_desc[i].current_reservation = vm_info_desc.CurrentReservation;
        }
    }

    std::vector<MemoryDesc> const& getLocalMemoryDescription() const
    {
        return m_local_memory_desc;
    }

    std::vector<MemoryDesc> const& getNonLocalMemoryDescription() const
    {
        return m_non_local_memory_desc;
    }

private:
    HwAdapter& m_enclosing;
    HwOutputEnumerator m_output_enumerator;
    uint32_t m_num_nodes;
    std::vector<MemoryDesc> m_local_memory_desc;
    std::vector<MemoryDesc> m_non_local_memory_desc;
};



HwAdapter::HwAdapter(GlobalSettings const& global_settings,
    ComPtr<IDXGIFactory6> const& adapter_factory,
    ComPtr<IDXGIAdapter4> const& adapter, bool enable_debug_mode) :
    m_global_settings{ global_settings },
    m_dxgi_adapter{ adapter },
    m_dxgi_adapter_factory{ adapter_factory },
    m_impl{ nullptr }
{
    DXGI_ADAPTER_DESC3 desc;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        adapter->GetDesc3(&desc),
        S_OK
    );

    m_properties.details.name = desc.Description;
    m_properties.details.vendor_id = desc.VendorId;
    m_properties.details.device_id = desc.DeviceId;
    m_properties.details.sub_system_id = desc.SubSysId;
    m_properties.details.revision = desc.Revision;
    m_properties.details.dedicated_video_memory = static_cast<size_t>(desc.DedicatedVideoMemory);
    m_properties.details.dedicated_system_memory = static_cast<size_t>(desc.DedicatedSystemMemory);
    m_properties.details.shared_system_memory = static_cast<size_t>(desc.SharedSystemMemory);
    m_properties.details.luid.LowPart = desc.AdapterLuid.LowPart;
    m_properties.details.luid.HighPart = desc.AdapterLuid.HighPart;
    m_properties.details.graphics_preemption_granularity = static_cast<DxgiGraphicsPreemptionGranularity>(desc.GraphicsPreemptionGranularity);
    m_properties.details.compute_preemption_granularity = static_cast<DxgiComputePreemptionGranularity>(desc.ComputePreemptionGranularity);


    // attempt to create D3D12 device in order to determine feature level of the hardware and receive information regarding the memory budgets

    ComPtr<ID3D12Device> d3d12_device{ nullptr };
    for (auto feature_level : { std::make_pair(D3D12FeatureLevel::_12_1, "12.1"), std::make_pair(D3D12FeatureLevel::_12_0, "12.0"),
        std::make_pair(D3D12FeatureLevel::_11_1, "11.1"), std::make_pair(D3D12FeatureLevel::_11_0, "11.0") })
    {
        if (D3D12CreateDevice(m_dxgi_adapter.Get(), static_cast<D3D_FEATURE_LEVEL>(feature_level.first), IID_PPV_ARGS(&d3d12_device)) == S_OK)
        {
            logger().out("Device \"" + wstringToAsciiString(m_properties.details.name) +
                "\": feature level " + feature_level.second + " detected", LogMessageType::information);

            m_impl.reset(new impl{ *this, m_dxgi_adapter, desc.AdapterLuid, d3d12_device->GetNodeCount() });


            m_properties.d3d12_feature_level = feature_level.first;
            m_properties.num_nodes = d3d12_device->GetNodeCount();
            m_impl->refreshMemoryStatistics();
            m_properties.local = m_impl->getLocalMemoryDescription();
            m_properties.non_local = m_impl->getNonLocalMemoryDescription();


            setStringName(wstringToAsciiString(m_properties.details.name) + "_VendorID:" + std::to_string(m_properties.details.vendor_id)
                + "_DeviceID:" + std::to_string(m_properties.details.device_id) + "_SubSystemID:" + std::to_string(m_properties.details.sub_system_id)
                + "_Revision:" + std::to_string(m_properties.details.revision) + "_LUID:" + std::to_string(m_properties.details.luid.HighPart)
                + std::to_string(m_properties.details.luid.LowPart));

            break;
        }
    }


    if (d3d12_device)
    {
        m_device = d3d12::DeviceAttorney<HwAdapter>::makeDevice(d3d12_device, m_global_settings);
        m_device->setStringName("\"" + misc::wstringToAsciiString(m_properties.details.name)
            + "\"__D3D12_device");
    }
    else
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "no device compatible with Direct3D 12 found");
        return;
    }
}


HwAdapter::~HwAdapter() = default;


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
    m_impl->refreshMemoryStatistics();
    return m_properties;
}

HwOutputEnumerator const& HwAdapter::getOutputEnumerator() const
{
    return m_impl->getOutputEnumeratorForThisAdapter();
}

SwapChain HwAdapter::createSwapChain(osinteraction::windows::Window& window, SwapChainDescriptor const& desc) const
{
    return SwapChainAttorney<HwAdapter>::makeSwapChain(m_dxgi_adapter_factory, *m_device,
        m_device->defaultCommandQueue(), window, desc);
}

dx::d3d12::Device& HwAdapter::device() const
{
    return *m_device;
}