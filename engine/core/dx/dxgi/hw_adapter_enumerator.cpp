#include "hw_adapter_enumerator.h"
#include "engine/core/global_settings.h"
#include "engine/core/exception.h"
#include "engine/core/misc/log.h"
#include "engine/core/misc/misc.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/debug_interface.h"

using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::dxgi;
using namespace lexgine::core::misc;


HwAdapterEnumerator::HwAdapterEnumerator(GlobalSettings const& global_settings,
    DxgiGpuPreference enumeration_preference) :
    m_global_settings{ global_settings }
{
    ComPtr<IDXGIFactory2> dxgi_factory2;

    //Create DXGI factory

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        CreateDXGIFactory2(DebugInterface::retrieve() ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&dxgi_factory2)),
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

HwAdapterEnumerator::adapter_list_type::value_type& HwAdapterEnumerator::operator[](ptrdiff_t index)
{
    return m_adapter_list[index];
}

HwAdapterEnumerator::adapter_list_type::value_type const& HwAdapterEnumerator::operator[](ptrdiff_t index) const
{
    return m_adapter_list[index];
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
            m_adapter_list.emplace_back(HwAdapterAttorney<HwAdapterEnumerator>::makeHwAdapter(m_global_settings, m_dxgi_factory6, dxgi_adapter4));
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

HwAdapter* HwAdapterEnumerator::getWARPAdapter() const { return m_adapter_list.back().get(); }

uint32_t HwAdapterEnumerator::getAdapterCount() const
{
    return static_cast<uint32_t>(m_adapter_list.size());
}


HwAdapter::HwAdapter(GlobalSettings const& global_settings,
    ComPtr<IDXGIFactory6> const& adapter_factory,
    ComPtr<IDXGIAdapter4> const& adapter)
    : m_global_settings{ global_settings }
    , m_dxgi_adapter_factory{ adapter_factory }
    , m_dxgi_adapter{ adapter }

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

    ComPtr<ID3D12Device6> d3d12_device{ nullptr };
    for (auto feature_level : { std::make_pair(D3D12FeatureLevel::_12_1, "12.1"), std::make_pair(D3D12FeatureLevel::_12_0, "12.0"),
        std::make_pair(D3D12FeatureLevel::_11_1, "11.1"), std::make_pair(D3D12FeatureLevel::_11_0, "11.0") })
    {
        if (D3D12CreateDevice(m_dxgi_adapter.Get(), static_cast<D3D_FEATURE_LEVEL>(feature_level.first), IID_PPV_ARGS(&d3d12_device)) == S_OK)
        {
            logger().out("Device \"" + wstringToAsciiString(m_properties.details.name) +
                "\": feature level " + feature_level.second + " detected", LogMessageType::information);

            m_output_enumerator.reset(new HwOutputEnumerator{ m_dxgi_adapter, desc.AdapterLuid });


            m_properties.d3d12_feature_level = feature_level.first;
            m_properties.num_nodes = d3d12_device->GetNodeCount();
            m_properties.local.resize(m_properties.num_nodes);
            m_properties.non_local.resize(m_properties.num_nodes);
            refreshMemoryStatistics();

            setStringName(wstringToAsciiString(m_properties.details.name) + "_VendorID:" + std::to_string(m_properties.details.vendor_id)
                + "_DeviceID:" + std::to_string(m_properties.details.device_id) + "_SubSystemID:" + std::to_string(m_properties.details.sub_system_id)
                + "_Revision:" + std::to_string(m_properties.details.revision) + "_LUID:" + std::to_string(m_properties.details.luid.HighPart)
                + std::to_string(m_properties.details.luid.LowPart));

            break;
        }
    }


    ComPtr<ID3D11Device5> d3d11_device5{ nullptr };
    ComPtr<ID3D11DeviceContext4> d3d11_device_context4{ nullptr };
    if (global_settings.isGpuAcceleratedTextureConversionEnabled()) 
    {
        // attempt to create D3D11 device for GPU-based texture conversion, when GPU-accelerated texture conversion is enabled
        ComPtr<ID3D11Device> d3d11_device{ nullptr };
        ComPtr<ID3D11DeviceContext> d3d11_device_context{ nullptr };
        D3D_FEATURE_LEVEL compatible_feature_levels[] =
        {
            D3D_FEATURE_LEVEL_11_1
        };

        HRESULT res = D3D11CreateDevice(m_dxgi_adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, NULL,
            NULL, compatible_feature_levels, static_cast<UINT>(sizeof(compatible_feature_levels) / sizeof(D3D_FEATURE_LEVEL)),
            D3D11_SDK_VERSION, d3d11_device.GetAddressOf(), NULL, d3d11_device_context.GetAddressOf());

        if (res == S_OK && d3d11_device && d3d11_device_context)
        {
            LEXGINE_LOG_ERROR_IF_FAILED(
                this,
                d3d11_device->QueryInterface(IID_PPV_ARGS(&d3d11_device5)),
                S_OK
            );

            LEXGINE_LOG_ERROR_IF_FAILED(
                this,
                d3d11_device_context->QueryInterface(IID_PPV_ARGS(&d3d11_device_context4)),
                S_OK
            );
        }
        else
        {
            Log::retrieve()->out("WARNING: GPU-accelerated texture conversion was toggled on, but creation of Direct3D 11 device has failed."
                " The system will fall back to CPU-based texture conversion", LogMessageType::exclamation);
        }
    }



    if (d3d12_device)
    {
        m_device = d3d12::DeviceAttorney<HwAdapter>::makeDevice(this, d3d12_device, m_global_settings);
        m_device->setStringName("\"" + misc::wstringToAsciiString(m_properties.details.name)
            + "\"__D3D12_device");
    }
    else
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "no device compatible with Direct3D 12 found");
        return;
    }

    if (d3d11_device5 && d3d11_device_context4)
    {
        d3d12::DeviceAttorney<HwAdapter>::defineD3d11HandlesForDevice(*m_device, d3d11_device5, d3d11_device_context4);
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

void HwAdapter::refreshMemoryStatistics() const
{
    for (uint32_t i = 0; i < m_properties.num_nodes; ++i)
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO vm_info_desc;

        LEXGINE_THROW_ERROR_IF_FAILED(
            *this,
            m_dxgi_adapter->QueryVideoMemoryInfo(i, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &vm_info_desc),
            S_OK
        );
        m_properties.local[i].total = vm_info_desc.Budget;
        m_properties.local[i].available = vm_info_desc.AvailableForReservation;
        m_properties.local[i].current_usage = vm_info_desc.CurrentUsage;
        m_properties.local[i].current_reservation = vm_info_desc.CurrentReservation;

        LEXGINE_THROW_ERROR_IF_FAILED(
            *this,
            m_dxgi_adapter->QueryVideoMemoryInfo(i, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &vm_info_desc),
            S_OK
        );
        m_properties.non_local[i].total = vm_info_desc.Budget;
        m_properties.non_local[i].available = vm_info_desc.AvailableForReservation;
        m_properties.non_local[i].current_usage = vm_info_desc.CurrentUsage;
        m_properties.non_local[i].current_reservation = vm_info_desc.CurrentReservation;
    }
}

HwAdapter::Properties HwAdapter::getProperties() const
{
    refreshMemoryStatistics();
    return m_properties;
}

HwOutputEnumerator HwAdapter::getOutputEnumerator() const
{
    return *m_output_enumerator;
}

lexgine::core::dx::dxgi::SwapChain HwAdapter::createSwapChain(osinteraction::windows::Window& window, SwapChainDescriptor const& desc) const
{
    return SwapChainAttorney<HwAdapter>::makeSwapChain(m_dxgi_adapter_factory, *m_device,
        m_device->defaultCommandQueue(), window, desc);
}

dx::d3d12::Device& HwAdapter::device() const
{
    return *m_device;
}