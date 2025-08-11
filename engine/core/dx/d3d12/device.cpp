#include "engine/core/exception.h"
#include "engine/core/global_settings.h"
#include "engine/core/profiling_services.h"
#include "engine/core/misc/misc.h"
#include "engine/core/dx/d3d12/debug_interface.h"
#include "command_list.h"
#include "query_cache.h"
#include "resource.h"

#include "device.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;


namespace {

bool isWindows11OrNewer()
{
    RTL_OSVERSIONINFOEXW version_info;
    version_info.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    HMODULE ntdll_handle = GetModuleHandle(L"ntdll.dll");
    if (!ntdll_handle) return false;
    using RtlGetVersionPtr = NTSTATUS(*)(PRTL_OSVERSIONINFOW);
    RtlGetVersionPtr rtl_get_version = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(ntdll_handle, "RtlGetVersion"));
    if (!rtl_get_version) return false;

    rtl_get_version(reinterpret_cast<PRTL_OSVERSIONINFOW>(&version_info));
    if (version_info.dwMajorVersion > 10 || version_info.dwMajorVersion == 10 && (version_info.dwMinorVersion > 0 || version_info.dwBuildNumber >= 22000))
    {
        return true;
    }

    return false;
}


}


Device::Device(dxgi::HwAdapter* owning_adapter_ptr, ComPtr<ID3D12Device6> const& device, lexgine::core::GlobalSettings const& global_settings)
    : m_owning_adapter_ptr{ owning_adapter_ptr }
    , m_device{ device }
    , m_frame_progress_tracker{ *this }
    , m_query_cache{ new QueryCache{ global_settings, *this } }
    , m_max_frames_in_flight{ global_settings.getMaxFramesInFlight() }
    , m_default_command_queue{ CommandQueueAttorney<Device>::makeCommandQueue(*this, WorkloadType::direct, 0x1, CommandQueuePriority::normal, CommandQueueFlags::base_values::none) }
    , m_async_command_queue{ CommandQueueAttorney<Device>::makeCommandQueue(*this, global_settings.isAsyncComputeEnabled() ? WorkloadType::compute : WorkloadType::direct, 0x1, CommandQueuePriority::normal, CommandQueueFlags::base_values::none) }
    , m_copy_command_queue{ CommandQueueAttorney<Device>::makeCommandQueue(*this, global_settings.isAsyncCopyEnabled() ? WorkloadType::copy : WorkloadType::direct, 0x1, CommandQueuePriority::normal, CommandQueueFlags::base_values::none) }
{
    m_default_command_queue.setStringName("default_command_queue");
    m_async_command_queue.setStringName("async_command_queue");
    m_copy_command_queue.setStringName("copy_command_queue");

    DebugInterface const* p_debug_interface = DebugInterface::retrieve();
    if (p_debug_interface)
    {
        LEXGINE_THROW_ERROR_IF_FAILED(
            this,
            m_device->QueryInterface(IID_PPV_ARGS(&m_debug_device)),
            S_OK
        );
    }
}

FeatureD3D12Options Device::queryFeatureD3D12Options() const
{
    if (m_features) {
        return *m_features;
    }

    FeatureD3D12Options rv{};

    {
        // D3D12_FEATURE_DATA_D3D12_OPTIONS

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
    }

    {
        // D3D12_FEATURE_DATA_D3D12_OPTIONS1

        D3D12_FEATURE_DATA_D3D12_OPTIONS1 feature_desc;
        LEXGINE_THROW_ERROR_IF_FAILED(
            this,
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS1)),
            S_OK
        );
        rv.waveOps = feature_desc.WaveOps;
        rv.waveLaneCountMin = static_cast<uint32_t>(feature_desc.WaveLaneCountMin);
        rv.waveLaneCountMax = static_cast<uint32_t>(feature_desc.WaveLaneCountMax);
        rv.totalLaneCount = static_cast<uint32_t>(feature_desc.TotalLaneCount);
        rv.expandedComputeResourceStates = feature_desc.ExpandedComputeResourceStates;
        rv.int64ShaderOps = feature_desc.Int64ShaderOps;
    }

    {
        // D3D12_FEATURE_DATA_D3D12_OPTIONS2

        D3D12_FEATURE_DATA_D3D12_OPTIONS2 feature_desc;
        LEXGINE_THROW_ERROR_IF_FAILED(
            this,
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS2)),
            S_OK
        );
        rv.depthBoundsTestSupported = feature_desc.DepthBoundsTestSupported;
        rv.programmableSamplePositionsTier = static_cast<D3D12ProgrammableSamplePositionsTier>(feature_desc.ProgrammableSamplePositionsTier);
    }

    {
        // D3D12_FEATURE_DATA_D3D12_OPTIONS3

        D3D12_FEATURE_DATA_D3D12_OPTIONS3 feature_desc;
        LEXGINE_THROW_ERROR_IF_FAILED(
            this,
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS3)),
            S_OK
        );
        rv.copyQueueTimestampQueriesSupported = feature_desc.CopyQueueTimestampQueriesSupported;
        rv.castingFullyTypedFormatSupported = feature_desc.CastingFullyTypedFormatSupported;
        rv.writeBufferImmediateSupportFlags = misc::Flags<D3D12CommandListSupportFlags>{ feature_desc.WriteBufferImmediateSupportFlags };
        rv.viewInstancingTier = static_cast<D3D12ViewInstancingTier>(feature_desc.ViewInstancingTier);
        rv.barycentricsSupported = feature_desc.BarycentricsSupported;

    }

    {
        // D3D12_FEATURE_DATA_D3D12_OPTIONS4

        D3D12_FEATURE_DATA_D3D12_OPTIONS4 feature_desc;
        LEXGINE_THROW_ERROR_IF_FAILED(
            this,
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS4)),
            S_OK
        );
        rv.msaa64KBAlignedTextureSupported = feature_desc.MSAA64KBAlignedTextureSupported;
        rv.sharedResourceCompatibilityTier = static_cast<D3D12SharedResourceCompatibilityTier>(feature_desc.SharedResourceCompatibilityTier);
        rv.native16BitShaderOpsSupported = feature_desc.Native16BitShaderOpsSupported;
    }

    {
        // D3D12_FEATURE_DATA_D3D12_OPTIONS5

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 feature_desc;
        LEXGINE_THROW_ERROR_IF_FAILED(
            this,
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5)),
            S_OK
        );
        rv.srvOnlyTiledResourceTier3 = feature_desc.SRVOnlyTiledResourceTier3;
        rv.renderPassesTier = static_cast<D3D12RenderPassTier>(feature_desc.RenderPassesTier);
        rv.raytracingTier = static_cast<D3D12RaytracingTier>(feature_desc.RaytracingTier);
    }

    {
        // D3D12_FEATURE_DATA_D3D12_OPTIONS6

        D3D12_FEATURE_DATA_D3D12_OPTIONS6 feature_desc;
        LEXGINE_THROW_ERROR_IF_FAILED(
            this,
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6)),
            S_OK
        );
        rv.additionalShadingRatesSupported = feature_desc.AdditionalShadingRatesSupported;
        rv.perPrimitiveShadingRateSupportedWithViewportIndexing = feature_desc.PerPrimitiveShadingRateSupportedWithViewportIndexing;
        rv.variableShadingRateTier = static_cast<D3D12VariableShadingRateTier>(feature_desc.VariableShadingRateTier);
        rv.shadingRateImageTileSize = feature_desc.ShadingRateImageTileSize;
        rv.backgroundProcessingSupported = feature_desc.BackgroundProcessingSupported;
    }

    {
        // D3D12_FEATURE_DATA_D3D12_OPTIONS7

        D3D12_FEATURE_DATA_D3D12_OPTIONS7 feature_desc;
        LEXGINE_THROW_ERROR_IF_FAILED(
            this,
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7)),
            S_OK
        );
        rv.meshShaderTier = static_cast<D3D12MeshShaderTier>(feature_desc.MeshShaderTier);
        rv.samplerFeedbackTier = static_cast<D3D12SamplerFeedbackTier>(feature_desc.SamplerFeedbackTier);
    }


    if (isWindows11OrNewer())
    {

        {
            // D3D12_FEATURE_DATA_D3D12_OPTIONS8

            D3D12_FEATURE_DATA_D3D12_OPTIONS8 feature_desc;
            LEXGINE_THROW_ERROR_IF_FAILED(
                this,
                m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS8, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS8)),
                S_OK
            );
            rv.unalignedBlockTexturesSupported = feature_desc.UnalignedBlockTexturesSupported;
        }

        {
            // D3D12_FEATURE_DATA_D3D12_OPTIONS9

            D3D12_FEATURE_DATA_D3D12_OPTIONS9 feature_desc;
            LEXGINE_THROW_ERROR_IF_FAILED(
                this,
                m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS9)),
                S_OK
            );
            rv.meshShaderPipelineStatsSupported = feature_desc.MeshShaderPipelineStatsSupported;
            rv.meshShaderSupportsFullRangeRenderTargetArrayIndex = feature_desc.MeshShaderSupportsFullRangeRenderTargetArrayIndex;
            rv.atomicInt64OnTypedResourceSupported = feature_desc.AtomicInt64OnTypedResourceSupported;
            rv.atomicInt64OnGroupSharedSupported = feature_desc.AtomicInt64OnGroupSharedSupported;
            rv.derivativesInMeshAndAmplificationShadersSupported = feature_desc.DerivativesInMeshAndAmplificationShadersSupported;
            rv.waveMMATier = static_cast<D3D12WaveMMATier>(feature_desc.WaveMMATier);
        }

        {
            // D3D12_FEATURE_DATA_D3D12_OPTIONS10

            D3D12_FEATURE_DATA_D3D12_OPTIONS10 feature_desc;
            LEXGINE_THROW_ERROR_IF_FAILED(
                this,
                m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS10, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS10)),
                S_OK
            );
            rv.variableRateShadingSumCombinerSupported = feature_desc.VariableRateShadingSumCombinerSupported;
            rv.meshShaderPerPrimitiveShadingRateSupported = feature_desc.MeshShaderPerPrimitiveShadingRateSupported;
        }

        {
            // D3D12_FEATURE_DATA_D3D12_OPTIONS11

            D3D12_FEATURE_DATA_D3D12_OPTIONS11 feature_desc;
            LEXGINE_THROW_ERROR_IF_FAILED(
                this,
                m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS11, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS11)),
                S_OK
            );
            rv.atomicInt64OnDescriptorHeapResourceSupported = feature_desc.AtomicInt64OnDescriptorHeapResourceSupported;
        }

        {
            // D3D12_FEATURE_DATA_D3D12_OPTIONS12

            D3D12_FEATURE_DATA_D3D12_OPTIONS12 feature_desc;
            LEXGINE_THROW_ERROR_IF_FAILED(
                this,
                m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS12)),
                S_OK
            );
            rv.msPrimitivesPipelineStatisticIncludesCulledPrimitives = static_cast<D3D12TriState>(feature_desc.MSPrimitivesPipelineStatisticIncludesCulledPrimitives);
            rv.enhancedBarriersSupported = feature_desc.EnhancedBarriersSupported;
            rv.relaxedFormatCastingSupported = feature_desc.RelaxedFormatCastingSupported;
        }

        {
            // D3D12_FEATURE_DATA_D3D12_OPTIONS13

            D3D12_FEATURE_DATA_D3D12_OPTIONS13 feature_desc;
            LEXGINE_THROW_ERROR_IF_FAILED(
                this,
                m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13, &feature_desc, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS13)),
                S_OK
            );
            rv.unrestrictedBufferTextureCopyPitchSupported = feature_desc.UnrestrictedBufferTextureCopyPitchSupported;
            rv.unrestrictedVertexElementAlignmentSupported = feature_desc.UnrestrictedVertexElementAlignmentSupported;
            rv.invertedViewportHeightFlipsYSupported = feature_desc.InvertedViewportHeightFlipsYSupported;
            rv.invertedViewportDepthFlipsZSupported = feature_desc.InvertedViewportDepthFlipsZSupported;
            rv.textureCopyBetweenDimensionsSupported = feature_desc.TextureCopyBetweenDimensionsSupported;
            rv.alphaBlendFactorSupported = feature_desc.AlphaBlendFactorSupported;
        }
    }

    m_features.reset(new FeatureD3D12Options{ rv });

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

    rv.tile_based_renderer = feature_desc.TileBasedRenderer == TRUE;
    rv.UMA = feature_desc.UMA == TRUE;
    rv.cache_coherent_UMA = feature_desc.CacheCoherentUMA == TRUE;

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
    rv.num_quality_levels = feature_desc.NumQualityLevels;

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

    rv.max_gpu_virtual_address_bits_per_resource = feature_desc.MaxGPUVirtualAddressBitsPerResource;
    rv.max_gpu_virtual_address_bits_per_process = feature_desc.MaxGPUVirtualAddressBitsPerProcess;

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

ComPtr<ID3D11Device5> Device::nativeD3d11() const
{
    return m_d3d11_device;
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

Device::~Device() = default;



