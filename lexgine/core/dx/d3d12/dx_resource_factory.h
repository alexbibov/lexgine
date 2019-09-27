#ifndef LEXGINE_CORE_DX_D3D12_DX_RESOURCE_FACTORY_H
#define LEXGINE_CORE_DX_D3D12_DX_RESOURCE_FACTORY_H

#include <vector>
#include <unordered_map>
#include <array>

#include "lexgine/core/global_settings.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/dxgi/hw_adapter_enumerator.h"
#include "lexgine/core/dx/dxcompilation/dx_compiler_proxy.h"

namespace lexgine::core::dx::d3d12 {

struct UploadHeapPartition
{
    size_t offset;
    size_t size;
};

//! Used to create and encapsulate reused Direct3D resources
class DxResourceFactory final
{
public:
    DxResourceFactory(GlobalSettings const& global_settings,
        bool enable_debug_mode,
        dxgi::HwAdapterEnumerator::DxgiGpuPreference enumeration_preference);

    ~DxResourceFactory();

    dxgi::HwAdapterEnumerator const& hardwareAdapterEnumerator() const;
    dxcompilation::DXCompilerProxy& shaderModel6xDxCompilerProxy();
    DebugInterface const* debugInterface() const;

    DescriptorHeap& retrieveDescriptorHeap(Device const& device, DescriptorHeapType descriptor_heap_type, uint32_t page_id);
    Heap& retrieveUploadHeap(Device const& device);

    dxgi::HwAdapter const* retrieveHwAdapterOwningDevicePtr(Device const& device) const;

    /*! Attempts to allocate a new named section in the given upload heap.
     Returns details of the new allocation in case of success or an empty misc::Optional<T>
     container if a section with the desired name already existed in the upload heap.
    */
    misc::Optional<UploadHeapPartition> allocateSectionInUploadHeap(Heap const& upload_heap, std::string const& section_name, size_t section_size);

    /*! Attempts to retrieve a named section from the given upload heap.
     Returns the allocation details of the named section in case of success or an empty
     misc::Optional<T> container if the section with the required name is not found within
     the partitioning of the given upload heap.
    */
    misc::Optional<UploadHeapPartition> retrieveUploadHeapSection(Heap const& upload_heap, std::string const& section_name) const;

private:
    using descriptor_heap_page_pool = std::array<std::vector<std::unique_ptr<DescriptorHeap>>, 4U>;
    
    struct upload_heap_partitioning
    {
        size_t partitioned_space_size = 0ULL;
        std::unordered_map<misc::HashedString, UploadHeapPartition> partitioning;
    };

private:
    GlobalSettings const& m_global_settings;
    dx::d3d12::DebugInterface const* m_debug_interface;
    dxgi::HwAdapterEnumerator m_hw_adapter_enumerator;
    dxcompilation::DXCompilerProxy m_dxc_proxy;
    
    std::unordered_map<Device const*, descriptor_heap_page_pool> m_descriptor_heaps;
    std::unordered_map<Device const*, Heap> m_upload_heaps;
    std::unordered_map<Heap const*, upload_heap_partitioning> m_upload_heap_partitions;
};

}


#endif
