#include "resource.h"

#include "constant_buffer.h"

using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core;

ConstantBuffer::ConstantBuffer(Device const& device, uint64_t size,
    uint32_t node_mask/* = 0x1*/, uint32_t node_exposure_mask/* = 0x1*/,
    bool allow_cross_adapter/* = false*/)
{
    ResourceFlags creation_flags = allow_cross_adapter ? ResourceFlags::enum_type::allow_cross_adapter : ResourceFlags::enum_type::none;
    ResourceDescriptor buffer_descriptor = ResourceDescriptor::CreateBuffer(size, creation_flags);

    m_resource = std::make_unique<CommittedResource>(device, ResourceState::enum_type::generic_read,
        misc::Optional<ResourceOptimizedClearValue>{}, buffer_descriptor, AbstractHeapType::upload,
        HeapCreationFlags::enum_type::allow_only_buffers, node_mask, node_exposure_mask);

    m_buffer_gpu_virtual_address = reinterpret_cast<uint64_t>(m_resource->map());
}

ConstantBuffer::~ConstantBuffer()
{
    m_resource->unmap();
}

uint64_t ConstantBuffer::mappingAddress(size_t offset) const
{
    return m_buffer_gpu_virtual_address + offset;
}
