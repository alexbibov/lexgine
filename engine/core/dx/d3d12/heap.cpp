#include "heap.h"
#include "device.h"
#include "engine/core/exception.h"

using namespace lexgine::core::dx::d3d12;

namespace {

D3D12_HEAP_DESC createNativeHeapDesc(uint64_t size, D3D12_HEAP_PROPERTIES const& heap_properties, bool is_msaa_supported, HeapCreationFlags flags)
{
    D3D12_HEAP_DESC heap_desc;
    heap_desc.SizeInBytes = size;
    heap_desc.Properties = heap_properties;
    heap_desc.Alignment = is_msaa_supported ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    heap_desc.Flags = static_cast<D3D12_HEAP_FLAGS>(flags.getValue());

    return heap_desc;
}

}

D3D12_HEAP_PROPERTIES Heap::createNativeHeapProperties(AbstractHeapType heap_type, uint32_t node_mask, uint32_t node_exposure_mask)
{
    D3D12_HEAP_PROPERTIES heap_properties;
    heap_properties.Type = static_cast<D3D12_HEAP_TYPE>(heap_type);
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_properties.CreationNodeMask = static_cast<UINT>(node_mask);
    heap_properties.VisibleNodeMask = static_cast<UINT>(node_exposure_mask);

    return heap_properties;
}

D3D12_HEAP_PROPERTIES Heap::createNativeHeapProperties(CPUPageProperty cpu_page_property, GPUMemoryPool gpu_memory_pool, uint32_t node_mask, uint32_t node_exposure_mask)
{
    D3D12_HEAP_PROPERTIES heap_properties;
    heap_properties.Type = D3D12_HEAP_TYPE_CUSTOM;
    heap_properties.CPUPageProperty = static_cast<D3D12_CPU_PAGE_PROPERTY>(cpu_page_property);
    heap_properties.MemoryPoolPreference = static_cast<D3D12_MEMORY_POOL>(gpu_memory_pool);
    heap_properties.CreationNodeMask = static_cast<UINT>(node_mask);
    heap_properties.VisibleNodeMask = static_cast<UINT>(node_exposure_mask);

    return heap_properties;
}

void Heap::retrieveAbstractHeapTypeProperties(Device const& device, AbstractHeapType heap_type, uint32_t node_mask, CPUPageProperty& out_cpu_page_property, GPUMemoryPool& out_gpu_memory_pool)
{
    D3D12_HEAP_PROPERTIES abstract_heap_architectural_properties = device.native()->GetCustomHeapProperties(node_mask, static_cast<D3D12_HEAP_TYPE>(heap_type));
    out_cpu_page_property = static_cast<CPUPageProperty>(abstract_heap_architectural_properties.CPUPageProperty);
    out_gpu_memory_pool = static_cast<GPUMemoryPool>(abstract_heap_architectural_properties.MemoryPoolPreference);
}

Device& Heap::device() const
{
    return m_device;
}

size_t Heap::capacity() const
{
    return static_cast<size_t>(m_size);
}

ComPtr<ID3D12Heap> Heap::native() const
{
    return m_heap;
}

bool Heap::isMSAASupported() const
{
    return m_is_msaa_supported;
}

CPUPageProperty Heap::getCPUPage() const
{
    return m_cpu_page;
}

GPUMemoryPool Heap::getGPUMemoryPool() const
{
    return m_gpu_pool;
}

uint32_t Heap::getResidentDevice() const
{
    return m_node_mask;
}

uint32_t Heap::getExposureMask() const
{
    return m_node_exposure_mask;
}


Heap::Heap(Device& device, AbstractHeapType type, uint64_t size, HeapCreationFlags flags, bool is_msaa_supported, uint32_t node_mask, uint32_t node_exposure_mask) :
    m_device{ device },
    m_size{ size },
    m_is_msaa_supported{ is_msaa_supported },
    m_node_mask{ node_mask },
    m_node_exposure_mask{ node_exposure_mask }
{
    retrieveAbstractHeapTypeProperties(device, type, node_mask, m_cpu_page, m_gpu_pool);

    D3D12_HEAP_PROPERTIES heap_properties = createNativeHeapProperties(type, node_mask, node_exposure_mask);
    D3D12_HEAP_DESC heap_desc = createNativeHeapDesc(size, heap_properties, is_msaa_supported, flags);

    LEXGINE_THROW_ERROR_IF_FAILED(this,
        device.native()->CreateHeap(&heap_desc, IID_PPV_ARGS(&m_heap)),
        S_OK);
}

Heap::Heap(Device& device, CPUPageProperty cpu_page_property, GPUMemoryPool gpu_memory_pool, uint64_t size, HeapCreationFlags flags, bool is_msaa_supported, uint32_t node_mask, uint32_t node_exposure_mask) :
    m_device{ device },
    m_size{ size },
    m_is_msaa_supported{ is_msaa_supported },
    m_cpu_page{ cpu_page_property },
    m_gpu_pool{ gpu_memory_pool },
    m_node_mask{ node_mask },
    m_node_exposure_mask{ node_exposure_mask }
{
    D3D12_HEAP_PROPERTIES heap_properties = createNativeHeapProperties(cpu_page_property, gpu_memory_pool, node_mask, node_exposure_mask);
    D3D12_HEAP_DESC heap_desc = createNativeHeapDesc(size, heap_properties, is_msaa_supported, flags);

    LEXGINE_THROW_ERROR_IF_FAILED(
        this,
        device.native()->CreateHeap(&heap_desc, IID_PPV_ARGS(&m_heap)),
        S_OK
    );
}