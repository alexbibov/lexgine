#ifndef LEXGINE_CORE_DX_D3D12_DEVICE_H
#define LEXGINE_CORE_DX_D3D12_DEVICE_H

#include <memory>
#include <array>

#include "common.h"

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/misc/flags.h"
#include "engine/core/entity.h"
#include "engine/core/class_names.h"
#include "engine/core/dx/dxgi/lexgine_core_dx_dxgi_fwd.h"

#include "lexgine_core_dx_d3d12_fwd.h"
#include "root_signature.h"
#include "root_signature_cache.h"
#include "heap.h"
#include "descriptor_heap.h"
#include "command_queue.h"
#include "command_allocator_ring.h"
#include "frame_progress_tracker.h"



using namespace Microsoft::WRL;


namespace lexgine::core::dx::d3d12 {

template<typename T> class DeviceAttorney;


//! Shader precision modes that can be supported by the graphics hardware
BEGIN_FLAGS_DECLARATION(ShaderPrecisionMode)
FLAG(_32_bit, D3D12_SHADER_MIN_PRECISION_SUPPORT_NONE)
FLAG(_10_bit, D3D12_SHADER_MIN_PRECISION_SUPPORT_10_BIT)
FLAG(_16_bit, D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT)
END_FLAGS_DECLARATION(ShaderPrecisionMode)


//! Specifies whether the hardware and driver support tiled resources
enum class D3D12TiledResourceTier
{
    not_supported = D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED,    //!< textures cannot be created with D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE layout
    tier1 = D3D12_TILED_RESOURCES_TIER_1,    //!< 2D textures can be created with D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE layout with some limitations on formats and properties
    tier2 = D3D12_TILED_RESOURCES_TIER_2,    //!< indicates that a superset of the tier1 features is supported including support for some additional functionality (see D3D12_TILED_RESOURCES_TIER enumeration on MSDN)
    tier3 = D3D12_TILED_RESOURCES_TIER_3,    //!< indicated that s superset of the tier2 features is supported including support for 3D texture tiles (see D3D12_TILED_RESOURCES_TIER enumeration on MSDN for further details)
};


//! Specifies the level at which the hardware and driver support resource binding (see Hardware Tiers on MSDN)
enum class D3D12ResourceBindingTier
{
    tier1 = D3D12_RESOURCE_BINDING_TIER_1,
    tier2 = D3D12_RESOURCE_BINDING_TIER_2,
    tier3 = D3D12_RESOURCE_BINDING_TIER_3
};


//! Identifies the tier level of conservative rasterization
enum class D3D12ConservativeRasterization
{
    not_supported = D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED,
    tier1 = D3D12_CONSERVATIVE_RASTERIZATION_TIER_1,
    tier2 = D3D12_CONSERVATIVE_RASTERIZATION_TIER_2,
    tier3 = D3D12_CONSERVATIVE_RASTERIZATION_TIER_3
};


//! Specifies the level of sharing across nodes of an adapter
enum class D3D12CrossNodeSharingTier
{
    not_supported = D3D12_CROSS_NODE_SHARING_TIER_NOT_SUPPORTED,
    tier1_emulated = D3D12_CROSS_NODE_SHARING_TIER_1_EMULATED,
    tier1 = D3D12_CROSS_NODE_SHARING_TIER_1,
    tier2 = D3D12_CROSS_NODE_SHARING_TIER_2
};


//! Specifies the level at which the hardware and driver require heap attribution related to resource type
enum class D3D12ResourceHeapTier
{
    tier1 = D3D12_RESOURCE_HEAP_TIER_1,
    tier2 = D3D12_RESOURCE_HEAP_TIER_2
};


//! Thin wrapper over D3D12_FEATURE_DATA_D3D12_OPTIONS
struct FeatureD3D12Options final
{
    bool doublePrecisionFloatShaderOps;
    bool outputMergerLogicOp;
    ShaderPrecisionMode minPrecisionSupport;
    D3D12TiledResourceTier tiledResourcesTier;
    D3D12ResourceBindingTier resourceBindingTier;
    bool PSSpecifiedStencilRefSupported;
    bool typedUAVLoadAdditionalFormats;
    bool ROVsSupported;
    D3D12ConservativeRasterization conservativeRasterizationTier;
    //uint32_t MaxGPUVirtualAddressBitsPerResource;
    bool standardSwizzle64KBSupported;
    D3D12CrossNodeSharingTier crossNodeSharingTier;
    bool crossAdapterRowMajorTextureSupported;
    bool VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
    D3D12ResourceHeapTier resourceHeapTier;
};

//! Thin wrapper over D3D12_FEATURE_DATA_ARCHITECTURE
struct FeatureArchitecture final
{
    bool tileBasedRenderer;
    bool UMA;
    bool cacheCoherentUMA;
};

//! Thin wrapper over D3D12_FEATURE_DATA_FORMAT_SUPPORT
struct FeatureFormatSupport final
{
    D3D12_FORMAT_SUPPORT1 support1;
    D3D12_FORMAT_SUPPORT2 support2;
};

//! Specifies options for determining quality levels
BEGIN_FLAGS_DECLARATION(MultisampleQualityLevels)
FLAG(none, D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE)
FLAG(tiled_resource, D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_TILED_RESOURCE)
END_FLAGS_DECLARATION(MultisampleQualityLevels)

//! Thin wrapper over D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS
struct FeatureMultisampleQualityLevels final
{
    MultisampleQualityLevels flags;
    uint32_t numQualityLevels;
};

//! Thin wrapper over D3D12_GPU_VIRTUAL_ADDRESS_SUPPORT
struct FeatureGPUVirtualAddressSupport final
{
    uint32_t maxGPUVirtualAddressBitsPerResource;
    uint32_t maxGPUVirtualAddressBitsPerProcess;
};


/*! Thin wrapper over ID3D12Device interface.
 Note that this class is subject for continuous changing: new functionality may be added at any time
 in order to provide convenience APIs for the basic Direc3D12 functionality. All features provided by this
 class can be emulated by calling native() and then using the basic APIs of ID3D12Device
 */
class Device final : public NamedEntity<class_names::D3D12_Device>
{
    friend class DeviceAttorney<dxgi::HwAdapter>;

public:
    FeatureD3D12Options queryFeatureD3D12Options() const;    //! queries Direc3D12 feature options in the current graphics driver

    //! queries details about adapter's architecture for the physical adapter corresponding to provided @param node_index
    FeatureArchitecture queryFeatureArchitecture(uint32_t node_index) const;

    FeatureFormatSupport queryFeatureFormatSupport(DXGI_FORMAT format) const;    //! queries information regarding support of a specific format

    //! queries multi-sample quality levels for requested @param format and @param sample_count
    FeatureMultisampleQualityLevels queryFeatureQualityLevels(DXGI_FORMAT format, uint32_t sample_count) const;

    FeatureGPUVirtualAddressSupport queryFeatureGPUVirtualAddressSupport() const;    //! queries details on the adapter's GPU virtual address space limitations, including maximum address bits per resource and per process

    uint32_t getNodeCount() const;

    ComPtr<ID3D12Device> native() const;    //! returns native IDirect3D12 interface for the device

    void setStringName(std::string const& entity_string_name) override;	//! sets new user-friendly string name for the Direct3D 12 device

    ComPtr<ID3D12RootSignature> createRootSignature(D3DDataBlob const& serialized_root_signature, std::string const& root_signature_friendly_name, uint32_t node_mask = 1);    //! creates native Direct3D 12 root signature interface based on serialized root signature data
    ComPtr<ID3D12RootSignature> retrieveRootSignature(std::string const& root_signature_friendly_name, uint32_t node_mask = 1);

    Fence createFence(FenceSharing sharing = FenceSharing::none);    //! creates synchronization fence

    std::unique_ptr<DescriptorHeap> createDescriptorHeap(DescriptorHeapType type, uint32_t num_descriptors, uint32_t node_mask = 0);    //! creates descriptor heap

    /*! creates heap of one of the abstract types. Here parameter node_mask identifies the node where the heap will reside (exactly one bit in the mask corresponding to the target node should be set),
     and node_exposure_mask identifies, which nodes will "see" the heap (the bits corresponding to these nodes should be set)
     */
    Heap createHeap(AbstractHeapType type, uint64_t size, HeapCreationFlags flags = HeapCreationFlags{ HeapCreationFlags::base_values::allow_all },
        bool is_msaa_supported = false, uint32_t node_mask = 0, uint32_t node_exposure_mask = 0);

    /*! creates custom heap with requested CPU memory page properties and allocated from the given GPU memory pool. Here @param node_mask identifies the node where
     the heap will reside (exactly one bit in the mask corresponding to the target node should be set),
     and node_exposure_mask identifies, which nodes will "see" the heap (the bits corresponding to these nodes should be set)
     */
    Heap createHeap(CPUPageProperty cpu_page_property, GPUMemoryPool gpu_memory_pool, uint64_t size, HeapCreationFlags flags = HeapCreationFlags{ HeapCreationFlags::base_values::allow_all },
        bool is_msaa_supported = false, uint32_t node_mask = 0, uint32_t node_exposure_mask = 0);

    uint32_t maxFramesInFlight() const;    //! maximal number of frames allowed to be simultaneously rendered on this device

    CommandQueue const& defaultCommandQueue() const;    //! graphics command queue associated with this device
    CommandQueue const& asyncCommandQueue() const;    //! asynchronous compute command queue associated with this device (may coincide with default command queue depending on the hardware and settings)
    CommandQueue const& copyCommandQueue() const;    //! copy command queue associated with this device

    //! Creates new command list, which is closed immediately after creation. Therefore, it has to be reset before usage
    CommandList createCommandList(CommandType command_list_workload_type, uint32_t node_mask,
        FenceSharing command_list_sync_mode = FenceSharing::none, PipelineState const* initial_pipeline_state = nullptr);

    FrameProgressTracker& frameProgressTracker() { return m_frame_progress_tracker; }
    FrameProgressTracker const& frameProgressTracker() const { return frameProgressTracker(); }

    QueryCache* queryCache() const { return m_query_cache.get(); }

    Device(Device const&) = delete;
    Device(Device&&) = delete;

    ~Device();

private:
    Device(ComPtr<ID3D12Device6> const& native_device, lexgine::core::GlobalSettings const& global_settings);

private:
    ComPtr<ID3D12Device6> m_device;    //!< encapsulated pointer to Direct3D12 device interface
    ComPtr<ID3D12DebugDevice2> m_debug_device;    //!< interface used by GPU based validation
    RootSignatureCache m_rs_cache;    //!< cached root signatures

    FrameProgressTracker m_frame_progress_tracker;
    std::unique_ptr<QueryCache> m_query_cache;    //!< cache structure containing device query data

    uint32_t m_max_frames_in_flight;
    CommandQueue m_default_command_queue;
    CommandQueue m_async_command_queue;
    CommandQueue m_copy_command_queue;
};


template<> class DeviceAttorney<dxgi::HwAdapter>
{
    friend class dxgi::HwAdapter;

private:

    static std::unique_ptr<Device> makeDevice(ComPtr<ID3D12Device6> const& native_device_interface, lexgine::core::GlobalSettings const& global_settings)
    {
        return std::unique_ptr<Device>{new Device{ native_device_interface, global_settings }};
    }
};

}


#endif
