#include <cassert>

#include "cbv_descriptor.h"
#include "resource.h"

namespace lexgine::core::dx::d3d12
{

CBVDescriptor::CBVDescriptor(Resource const& resource,
    uint32_t offset_from_start, uint32_t view_size_in_bytes) 
    : m_resource_ref{ resource }
{
    auto resource_desc = resource.descriptor();

    assert(resource_desc.dimension == ResourceDimension::buffer);

    m_native.BufferLocation = resource.getGPUVirtualAddress() + offset_from_start;
    m_native.SizeInBytes = static_cast<UINT>(view_size_in_bytes);
}

D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDescriptor::nativeDescriptor() const
{
    return m_native;
}

Resource const& CBVDescriptor::associatedResource() const
{
    return m_resource_ref;
}

uint64_t CBVDescriptor::gpuVirtualAddress() const
{
    return m_native.SizeInBytes;
}

uint32_t CBVDescriptor::size() const
{
    return m_native.SizeInBytes;
}

}    // namespace lexgine::core::dx::d3d12