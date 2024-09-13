#ifndef LEXGINE_CORE_DX_D3D12_DX_RESOURCE_FACTORY_H
#define LEXGINE_CORE_DX_D3D12_DX_RESOURCE_FACTORY_H

#include <vector>
#include <unordered_map>
#include <array>

#include "engine/core/global_settings.h"
#include "engine/core/dx/dxgi/hw_adapter_enumerator.h"
#include "engine/core/dx/d3d12/d3d12_tools.h"
#include "engine/core/dx/d3d12/upload_buffer_allocator.h"
#include "engine/core/dx/dxcompilation/dx_compiler_proxy.h"
#include "engine/core/misc/hashed_string.h"
#include "lexgine_core_dx_d3d12_fwd.h"
#include "debug_interface.h"

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
        bool enable_debug_mode, GpuBasedValidationSettings const& gpu_based_validation_settings,
        dxgi::DxgiGpuPreference enumeration_preference);

    ~DxResourceFactory();

    dxgi::HwAdapterEnumerator const& hardwareAdapterEnumerator() const;
    dxcompilation::DXCompilerProxy& shaderModel6xDxCompilerProxy();

    DescriptorHeap& retrieveDescriptorHeap(Device const& device, DescriptorHeapType descriptor_heap_type, uint32_t page_id);
    Heap& retrieveUploadHeap(Device const& device);
    StaticDescriptorAllocationManager& getStaticAllocationManagerForDescriptorHeap(Device const& device, DescriptorHeapType descriptor_heap_type, uint32_t page_id);


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
    
    DxgiFormatFetcher const& dxgiFormatFetcher() const { return m_dxgiFormatFetcher; }

    size_t getUploadHeapFreeSpace(Heap const& upload_heap) const;    //!< Returns size of unallocated space in given upload heap
    
    size_t getUploadHeapFreeSpace(Device const& owning_device) const;    //!< Returns size of unallocated space in the upload heap owned by given device

private:
    struct descriptor_heap_page_pool
    {
        static constexpr size_t heap_type_count = static_cast<size_t>(DescriptorHeapType::count);
        std::array<std::vector<std::unique_ptr<DescriptorHeap>>, heap_type_count> heaps;
        std::array<std::vector<std::unique_ptr<StaticDescriptorAllocationManager>>, heap_type_count> static_allocators;
    };

    struct upload_heap_partitioning
    {
        size_t partitioned_space_size = 0ULL;
        std::unordered_map<misc::HashedString, UploadHeapPartition> partitioning;
    };

private:
    GlobalSettings const& m_global_settings;
    dxgi::HwAdapterEnumerator m_hw_adapter_enumerator;
    dxcompilation::DXCompilerProxy m_dxc_proxy;

    std::unordered_map<Device const*, descriptor_heap_page_pool> m_descriptor_heaps;
    std::unordered_map<Device const*, Heap> m_upload_heaps;
    std::unordered_map<Heap const*, upload_heap_partitioning> m_upload_heap_partitions;

    DxgiFormatFetcher const m_dxgiFormatFetcher;
    
};

}


#endif
