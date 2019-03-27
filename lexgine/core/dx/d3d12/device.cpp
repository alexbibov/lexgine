#include "device.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/dx/d3d12/debug_interface.h"

#include "command_list.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

Device::Device(ComPtr<ID3D12Device> const& device, lexgine::core::GlobalSettings const& global_settings) :
    m_device{ device },
    m_max_frames_in_flight{ global_settings.getMaxFramesInFlight() },
    m_default_command_queue{ CommandQueueAttorney<Device>::makeCommandQueue(*this, WorkloadType::direct, 0x1, CommandQueuePriority::normal, CommandQueueFlags::enum_type::none) },
    m_async_command_queue{ CommandQueueAttorney<Device>::makeCommandQueue(*this, global_settings.isAsyncComputeEnabled() ? WorkloadType::compute : WorkloadType::direct, 0x1, CommandQueuePriority::normal, CommandQueueFlags::enum_type::none) },
    m_copy_command_queue{ CommandQueueAttorney<Device>::makeCommandQueue(*this, global_settings.isAsyncCopyEnabled() ? WorkloadType::copy : WorkloadType::direct, 0x1, CommandQueuePriority::normal, CommandQueueFlags::enum_type::none) }
{
    m_default_command_queue.setStringName("default_command_queue");
    m_async_command_queue.setStringName("async_command_queue");
    m_copy_command_queue.setStringName("copy_command_queue");
}

Device::~Device()
{
    OutputDebugStringA(("Destroying device, ref. count " + std::to_string(m_device.Reset()) + "\n").c_str());
}

FeatureD3D12Options Device::queryFeatureD3D12Options() const
{
    FeatureD3D12Options rv{};

    D3D12_FEATURE_DATA_D3D12_OPTIONS feature_desc;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS)),
        S_OK
    );

    rv.doublePrecisionFloatShaderOps = feature_desc.DoublePrecisionFloatShaderOps == TRUE;
    rv.outputMergerLogicOp = feature_desc.OutputMergerLogicOp == TRUE;
    rv.minPrecisionSupport = feature_desc.MinPrecisionSupport;
    rv.tiledResourcesTier = static_cast<D3D12TiledResourceTier>(feature_desc.TiledResourcesTier);
    rv.resourceBindingTier = static_cast<D3D12ResourceBindingTier>(feature_desc.ResourceBindingTier);
    rv.PSSpecifiedStencilRefSupported = feature_desc.PSSpecifiedStencilRefSupported == TRUE;
    rv.typedUAVLoadAdditionalFormats = feature_desc.TypedUAVLoadAdditionalFormats == TRUE;
    rv.ROVsSupported = feature_desc.ROVsSupported == TRUE;
    rv.conservativeRasterizationTier = static_cast<D3D12ConservativeRasterization>(feature_desc.ConservativeRasterizationTier);
    rv.standardSwizzle64KBSupported = feature_desc.StandardSwizzle64KBSupported == TRUE;
    rv.crossNodeSharingTier = static_cast<D3D12CrossNodeSharingTier>(feature_desc.CrossNodeSharingTier);
    rv.crossAdapterRowMajorTextureSupported = feature_desc.CrossAdapterRowMajorTextureSupported == TRUE;
    rv.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation = feature_desc.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation == TRUE;
    rv.resourceHeapTier = static_cast<D3D12ResourceHeapTier>(feature_desc.ResourceHeapTier);

    return rv;
}

FeatureArchitecture Device::queryFeatureArchitecture(uint32_t node_index) const
{
    FeatureArchitecture rv{};

    D3D12_FEATURE_DATA_ARCHITECTURE feature_desc;
    feature_desc.NodeIndex = node_index;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &feature_desc, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE)),
        S_OK
    );

    rv.tileBasedRenderer = feature_desc.TileBasedRenderer == TRUE;
    rv.UMA = feature_desc.UMA == TRUE;
    rv.cacheCoherentUMA = feature_desc.CacheCoherentUMA == TRUE;

    return rv;
}

FeatureFormatSupport Device::queryFeatureFormatSupport(DXGI_FORMAT format) const
{
    FeatureFormatSupport rv{};

    D3D12_FEATURE_DATA_FORMAT_SUPPORT feature_desc;
    feature_desc.Format = format;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &feature_desc, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)),
        S_OK
    );

    rv.support1 = feature_desc.Support1;
    rv.support2 = feature_desc.Support2;

    return rv;
}

FeatureMultisampleQualityLevels Device::queryFeatureQualityLevels(DXGI_FORMAT format, uint32_t sample_count) const
{
    FeatureMultisampleQualityLevels rv{};

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS feature_desc;
    feature_desc.Format = format;
    feature_desc.SampleCount = sample_count;
    feature_desc.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &feature_desc, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS)),
        S_OK
    );

    rv.flags = feature_desc.Flags;
    rv.numQualityLevels = feature_desc.NumQualityLevels;

    return rv;
}

FeatureGPUVirtualAddressSupport Device::queryFeatureGPUVirtualAddressSupport() const
{
    FeatureGPUVirtualAddressSupport rv{};

    D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT feature_desc;
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &feature_desc, sizeof(D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT)),
        S_OK
    );

    rv.maxGPUVirtualAddressBitsPerResource = feature_desc.MaxGPUVirtualAddressBitsPerResource;
    rv.maxGPUVirtualAddressBitsPerProcess = feature_desc.MaxGPUVirtualAddressBitsPerProcess;

    return rv;
}

uint32_t Device::getNodeCount() const
{
    return static_cast<UINT>(m_device->GetNodeCount());
}

ComPtr<ID3D12Device> Device::native() const
{
    return m_device;
}

void Device::setStringName(std::string const& entity_string_name)
{
    Entity::setStringName(entity_string_name);
    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        m_device->SetName(misc::asciiStringToWstring(entity_string_name).c_str()),
        S_OK
    );

    m_default_command_queue.setStringName(entity_string_name + "__direct_cmd_queue");
    m_async_command_queue.setStringName(entity_string_name + "__compute_cmd_queue");
    m_copy_command_queue.setStringName(entity_string_name + "__copy_cmd_queue");
}

ComPtr<ID3D12RootSignature> Device::createRootSignature(lexgine::core::D3DDataBlob const& serialized_root_signature, std::string const& root_signature_friendly_name, uint32_t node_mask)
{
    return m_rs_cache.findOrCreate(*this, root_signature_friendly_name, node_mask, serialized_root_signature);
}

ComPtr<ID3D12RootSignature> Device::retrieveRootSignature(std::string const& root_signature_friendly_name, uint32_t node_mask)
{
    return m_rs_cache.find(root_signature_friendly_name, node_mask);
}

Fence Device::createFence(FenceSharing sharing/* = FenceSharing::none*/)
{
    return FenceAttorney<Device>::makeFence(*this, sharing);
}

std::unique_ptr<DescriptorHeap> Device::createDescriptorHeap(DescriptorHeapType type, uint32_t num_descriptors, uint32_t node_mask)
{
    return std::unique_ptr<DescriptorHeap>{new DescriptorHeap{ *this, type, num_descriptors, node_mask }};
}

Heap Device::createHeap(AbstractHeapType type, uint64_t size, HeapCreationFlags flags, bool is_msaa_supported, uint32_t node_mask, uint32_t node_exposure_mask)
{
    return Heap{ *this, type, size, flags, is_msaa_supported, node_mask, node_exposure_mask };
}

Heap Device::createHeap(CPUPageProperty cpu_page_property, GPUMemoryPool gpu_memory_pool, uint64_t size, HeapCreationFlags flags, bool is_msaa_supported, uint32_t node_mask, uint32_t node_exposure_mask)
{
    return Heap{ *this, cpu_page_property, gpu_memory_pool, size, flags, is_msaa_supported, node_mask, node_exposure_mask };
}

uint32_t Device::maxFramesInFlight() const
{
    return m_max_frames_in_flight;
}

CommandQueue const& Device::defaultCommandQueue() const
{
    return m_default_command_queue;
}

CommandQueue const& Device::asyncCommandQueue() const
{
    return m_async_command_queue;
}

CommandQueue const& Device::copyCommandQueue() const
{
    return m_copy_command_queue;
}

CommandList Device::createCommandList(CommandType command_list_workload_type, uint32_t node_mask,
    FenceSharing command_list_sync_mode/* = FenceSharing::none */,
    PipelineState const* initial_pipeline_state/* = nullptr */)
{
    return CommandListAttorney<Device>::makeCommandList(*this, command_list_workload_type,
        node_mask, command_list_sync_mode, initial_pipeline_state);
}



