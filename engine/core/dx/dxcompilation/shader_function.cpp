#include <format>

#include "engine/core/exception.h"
#include "engine/core/globals.h"
#include "engine/core/misc/log.h"
#include "engine/core/misc/misc.h"
#include "engine/core/dx/d3d12/bindless_descriptor_cache.h"
#include "engine/core/dx/d3d12/cbv_descriptor.h"
#include "engine/core/dx/d3d12/command_list.h"
#include "engine/core/dx/d3d12/descriptor_allocator.h"
#include "engine/core/dx/d3d12/descriptor_heap.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/sampler_descriptor.h"
#include "engine/core/dx/d3d12/uav_descriptor.h"
#include "engine/core/dx/d3d12/caches/root_signature_blob_cache.h"
#include "shader_function.h"
#include "shader_stage.h"


namespace lexgine::core::dx::dxcompilation {

namespace
{

uint32_t getDataTypeSize(StorageResourceDataType data_type)
{
    switch (data_type)
    {
    case StorageResourceDataType::unorm:
    case StorageResourceDataType::snorm:
    case StorageResourceDataType::sint:
    case StorageResourceDataType::uint:
    case StorageResourceDataType::float32:
        return 4;

    case StorageResourceDataType::float64:
    case StorageResourceDataType::continued:
        return 8;

    default:
        return 0;
    }
}

}  // namespace

ShaderFunction::ShaderFunction(Globals& globals, ShaderFunctionRootUniformBuffers const& flags/* = ShaderFunctionRootUniformBuffers::base_values::None*/)
    : ProvidesGlobals { globals }
    , m_device { globals.device() }
    , m_flags{ flags }
{
    // Create buffer for atomic counters
    d3d12::ResourceDescriptor counter_resource_desc = d3d12::ResourceDescriptor::createBuffer(D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT * c_max_uav_with_counters_count, d3d12::ResourceFlags::base_values::none);
    m_uav_atomic_counters = d3d12::CommittedResource {
        m_device,
        d3d12::ResourceState::base_values::unordered_access,
        misc::Optional<d3d12::ResourceOptimizedClearValue> {},
        counter_resource_desc,
        d3d12::AbstractHeapType::_default,
        d3d12::HeapCreationFlags::base_values::allow_all
    };
}

ShaderFunction::~ShaderFunction() = default;

ShaderStage* ShaderFunction::createShaderStage(d3d12::caches::HLSLShaderHandle shader_handle)
{
    std::unique_ptr<ShaderStage> new_shader_stage = ShaderStageAttorney<ShaderFunction>::createShaderStage(m_globals, shader_handle, this);
    ShaderType shader_type = new_shader_stage->getShaderType();
    size_t shader_type_id = static_cast<size_t>(shader_type);
    if (m_shader_stages[shader_type_id])
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Attempted to create shader stage, which already exists in the shader function");
    }

    m_shader_stages[shader_type_id] = std::move(new_shader_stage);
    m_shader_function_stale = true;

    return m_shader_stages[shader_type_id].get();
}

void ShaderFunction::collectInputResourceBindings()
{
    if (!m_shader_function_stale) {
        return;
    }

    m_reflected_declarations.clear();
    for (std::unique_ptr<ShaderStage> const& shader_stage : m_shader_stages)
    {
        if (ShaderStage* p_shader_stage = shader_stage.get())
        {
            p_shader_stage->build();
            std::vector<ReflectedDeclaration> stage_declarations = ShaderStageAttorney<ShaderFunction>::getShaderStageDeclarations(p_shader_stage);
            m_reflected_declarations.insert(m_reflected_declarations.end(), stage_declarations.begin(), stage_declarations.end());
        }
    }
}

d3d12::caches::RootSignatureHandle ShaderFunction::buildInputResourceBindings()
{
    if (!m_shader_function_stale)
    {
        return m_root_signature_handle;
    }

    std::vector<ShaderFunctionConstantBufferRootIds> root_uniform_ids;
    std::vector<uint32_t> root_constant_buffer_registers;
    for (int id = static_cast<int>(ShaderFunctionConstantBufferRootIds::scene_uniforms); id < static_cast<int>(ShaderFunctionConstantBufferRootIds::count); ++id)
    {
        if (m_flags.isSet(static_cast<ShaderFunctionRootUniformBuffers::base_values>(1 << id)))
        {
            root_uniform_ids.push_back(static_cast<ShaderFunctionConstantBufferRootIds>(id));
            root_constant_buffer_registers.push_back(static_cast<uint32_t>(id));
        }
    }

    BindingLayoutCompilationResult compilation_result = compileBindingLayout(m_reflected_declarations, root_constant_buffer_registers);
    if (std::string const* p_error = std::get_if<std::string>(&compilation_result))
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Unable to compile binding layout of the shader function: " + *p_error);
    }
    m_binding_layout = std::move(std::get<CompiledBindingLayout>(compilation_result));

    d3d12::RootSignature rs{};
    m_root_uniforms_to_rs_slots_mapping.clear();
    for (size_t i = 0; i < m_binding_layout->root_constant_buffers.size(); ++i)
    {
        RootConstantBufferLayout const& root_constant_buffer = m_binding_layout->root_constant_buffers[i];
        m_root_uniforms_to_rs_slots_mapping[root_uniform_ids[i]] = root_constant_buffer.root_slot;
        rs.addParameter(root_constant_buffer.root_slot,
            d3d12::RootEntryCBVDescriptor{ root_constant_buffer.shader_register, c_root_constant_buffer_register_space });
    }

    for (DescriptorTableLayout const& table : m_binding_layout->descriptor_tables)
    {
        rs.addParameter(table.root_slot, table.declaration, table.visibility);
    }
    m_occupied_rs_slots = static_cast<uint32_t>(m_binding_layout->root_constant_buffers.size() + m_binding_layout->descriptor_tables.size());

    d3d12::caches::RootSignatureBlobCache& rs_blob_cache = m_globals.rootSignatureBlobCache();
    d3d12::RootSignatureFlags rs_flags = d3d12::RootSignatureFlags::base_values::deny_vertex_shader
        | d3d12::RootSignatureFlags::base_values::deny_hull_shader
        | d3d12::RootSignatureFlags::base_values::deny_domain_shader
        | d3d12::RootSignatureFlags::base_values::deny_geometry_shader
        | d3d12::RootSignatureFlags::base_values::deny_pixel_shader;

    for (int shader_type_id = 0; shader_type_id < static_cast<int>(ShaderType::count); ++shader_type_id) {
        ShaderType shader_type = static_cast<ShaderType>(shader_type_id);
        if (!m_shader_stages[shader_type_id]) {
            continue;
        }

        switch (shader_type) {
        case ShaderType::vertex:
            rs_flags ^= d3d12::RootSignatureFlags::base_values::deny_vertex_shader;
            rs_flags |= d3d12::RootSignatureFlags::base_values::allow_input_assembler;
            break;
        case ShaderType::hull:
            rs_flags ^= d3d12::RootSignatureFlags::base_values::deny_hull_shader;
            break;
        case ShaderType::domain:
            rs_flags ^= d3d12::RootSignatureFlags::base_values::deny_domain_shader;
            break;
        case ShaderType::geometry:
            rs_flags ^= d3d12::RootSignatureFlags::base_values::deny_geometry_shader;
            break;
        case ShaderType::pixel:
            rs_flags ^= d3d12::RootSignatureFlags::base_values::deny_pixel_shader;
            break;
        }
    }

    m_root_signature_handle = rs_blob_cache.createRootSignatureBlobCompilationContract(std::move(rs), rs_flags);
    m_shader_function_stale = false;
    return m_root_signature_handle;
}

CompiledBindingLayout const& ShaderFunction::bindingLayout() const
{
    if (!m_binding_layout)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Binding layout of the shader function has not been built yet");
    }
    return *m_binding_layout;
}

std::optional<DescriptorTableId> ShaderFunction::findDescriptorTable(BindingDomain domain, d3d12::DescriptorHeapType heap_type) const
{
    std::vector<DescriptorTableLayout> const& tables = bindingLayout().descriptor_tables;
    for (size_t i = 0; i < tables.size(); ++i)
    {
        if (tables[i].domain == domain && tables[i].heap_type == heap_type)
        {
            return static_cast<DescriptorTableId>(i);
        }
    }

    return std::nullopt;
}

ShaderFunctionDescriptorTable ShaderFunction::createDescriptorTable(DescriptorTableId id, d3d12::DescriptorAllocator& allocator) const
{
    DescriptorTableLayout const& layout = bindingLayout().descriptor_tables.at(id);
    if (layout.domain == BindingDomain::bindless)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Bindless descriptor tables span the whole descriptor heap and cannot be allocated");
    }
    if (allocator.descriptorHeap().type() != layout.heap_type)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Descriptor allocator serves descriptor heap of type different from the one required by the descriptor table");
    }

    return ShaderFunctionDescriptorTable{ .id = id, .table = allocator.allocateDescriptorTable(layout.descriptor_count) };
}

void ShaderFunction::bindRootConstantBuffer(d3d12::CommandList& command_list,
    ShaderFunctionConstantBufferRootIds id,
    uint64_t gpu_virtual_address) const
{
    command_list.setRootConstantBufferView(m_root_uniforms_to_rs_slots_mapping.at(id), gpu_virtual_address);
}

void ShaderFunction::setDescriptorTable(d3d12::CommandList& command_list, ShaderFunctionDescriptorTable const& table) const
{
    DescriptorTableLayout const& layout = bindingLayout().descriptor_tables.at(table.id);
    command_list.setRootDescriptorTable(layout.root_slot, table.table.gpu_pointer);
}

void ShaderFunction::setBindlessDescriptorTables(d3d12::CommandList& command_list) const
{
    for (DescriptorTableLayout const& layout : bindingLayout().descriptor_tables)
    {
        if (layout.domain == BindingDomain::bindless)
        {
            command_list.setRootDescriptorTable(layout.root_slot, m_device.descriptorHeap(layout.heap_type).getBaseGPUPointer());
        }
    }
}

bool ShaderFunction::bindTexture(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, d3d12::Resource const& texture, uint32_t element/* = 0*/) const
{
    BindingPlacement const* p_placement = findPlacement(table, name, element);
    if (!p_placement)
    {
        return false;
    }

    std::optional<d3d12::SRVDescriptor> view = makeTextureView(*p_placement, name, texture);
    if (!view)
    {
        return false;
    }

    table.table.p_heap->createShaderResourceViewDescriptor(descriptorOffset(table, *p_placement, element), *view);
    return true;
}

bool ShaderFunction::bindTextureArray(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, d3d12::Resource const& texture,
    uint32_t first_array_element, uint32_t array_element_count, uint32_t element/* = 0*/) const
{
    BindingPlacement const* p_placement = findPlacement(table, name, element);
    if (!p_placement)
    {
        return false;
    }

    TextureShaderInputInfo const* p_info = std::get_if<TextureShaderInputInfo>(&p_placement->view);
    if (!p_info || p_info->resource_type != TextureResourceType::resource_with_dimension || !p_info->is_array
        || (p_info->dimension != ResourceDimension::texture1d && p_info->dimension != ResourceDimension::texture2d))
    {
        logger().out(std::format("Unable to bind texture array to shader input '{}': the input is not a texture array", name.string()), misc::LogMessageType::error);
        return false;
    }

    d3d12::SRVTextureArrayInfo info{};
    info.first_array_element = first_array_element;
    info.num_array_elements = array_element_count;
    table.table.p_heap->createShaderResourceViewDescriptor(descriptorOffset(table, *p_placement, element), d3d12::SRVDescriptor{ texture, info, p_info->is_cube });
    return true;
}

bool ShaderFunction::bindTextureBuffer(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, d3d12::Resource const& buffer,
    uint64_t first_buffer_element, uint32_t buffer_element_stride, uint32_t element/* = 0*/) const
{
    BindingPlacement const* p_placement = findPlacement(table, name, element);
    if (!p_placement)
    {
        return false;
    }

    TextureShaderInputInfo const* p_info = std::get_if<TextureShaderInputInfo>(&p_placement->view);
    if (!p_info || !p_info->is_buffer || p_info->resource_type == TextureResourceType::resource_with_dimension)
    {
        logger().out(std::format("Unable to bind buffer to shader input '{}': the input is not a tbuffer, structured buffer or raw buffer", name.string()),
            misc::LogMessageType::error);
        return false;
    }

    d3d12::ResourceDescriptor const& resource_desc = buffer.descriptor();
    if (resource_desc.dimension != d3d12::ResourceDimension::buffer)
    {
        logger().out(std::format("Unable to bind resource to shader input '{}': the resource is not a buffer", name.string()), misc::LogMessageType::error);
        return false;
    }

    uint32_t stride = buffer_element_stride;
    if (p_info->resource_type == TextureResourceType::tbuffer)
    {
        stride = getDataTypeSize(p_info->data_type);
    }
    else if (p_info->resource_type == TextureResourceType::raw_buffer)
    {
        stride = 4;
    }

    bool is_raw = p_info->resource_type == TextureResourceType::raw_buffer;
    d3d12::SRVBufferInfo info{
        .first_element = first_buffer_element,
        .num_elements = static_cast<uint32_t>((resource_desc.width - first_buffer_element * stride) / stride),
        .structure_byte_stride = p_info->resource_type == TextureResourceType::structured_buffer ? stride : 0,
        .flags = is_raw ? d3d12::SRVBufferInfoFlags::raw : d3d12::SRVBufferInfoFlags::none
    };

    d3d12::SRVDescriptor srv_descriptor{ buffer, info };
    if (is_raw)
    {
        srv_descriptor.overrideFormat(DXGI_FORMAT_R32_TYPELESS);
    }

    table.table.p_heap->createShaderResourceViewDescriptor(descriptorOffset(table, *p_placement, element), srv_descriptor);
    return true;
}

bool ShaderFunction::bindConstantBuffer(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, d3d12::Resource const& buffer,
    uint32_t offset_from_buffer_start, uint32_t size_in_bytes, uint32_t element/* = 0*/) const
{
    BindingPlacement const* p_placement = findPlacement(table, name, element);
    if (!p_placement)
    {
        return false;
    }

    if (p_placement->kind != ShaderInputKind::cbv)
    {
        logger().out(std::format("Unable to bind constant buffer to shader input '{}': the input is not a constant buffer", name.string()), misc::LogMessageType::error);
        return false;
    }

    table.table.p_heap->createConstantBufferViewDescriptor(descriptorOffset(table, *p_placement, element),
        d3d12::CBVDescriptor{ buffer, offset_from_buffer_start, size_in_bytes });
    return true;
}

bool ShaderFunction::bindStorageBlock(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, d3d12::Resource const& storage_block,
    uint64_t first_buffer_element, uint32_t buffer_element_stride, uint32_t element/* = 0*/)
{
    BindingPlacement const* p_placement = findPlacement(table, name, element);
    if (!p_placement)
    {
        return false;
    }

    StorageBlockShaderInputInfo const* p_info = std::get_if<StorageBlockShaderInputInfo>(&p_placement->view);
    if (!p_info)
    {
        logger().out(std::format("Unable to bind storage block to shader input '{}': the input is not an unordered access view", name.string()),
            misc::LogMessageType::error);
        return false;
    }

    size_t offset = descriptorOffset(table, *p_placement, element);
    d3d12::ResourceDescriptor const& resource_desc = storage_block.descriptor();
    switch (p_info->resource_type)
    {
    case StorageBlockResourceType::resource_with_dimension:
    {
        if (p_info->is_buffer)
        {
            logger().out(std::format("Unable to bind storage block to shader input '{}': typed buffer unordered access views are not supported", name.string()),
                misc::LogMessageType::error);
            return false;
        }

        if (p_info->is_array)
        {
            d3d12::UAVTextureArrayInfo info{};
            info.num_array_elements = static_cast<uint32_t>(resource_desc.depth);
            table.table.p_heap->createUnorderedAccessViewDescriptor(offset, d3d12::UAVDescriptor{ storage_block, info });
        }
        else
        {
            table.table.p_heap->createUnorderedAccessViewDescriptor(offset, d3d12::UAVDescriptor{ storage_block, d3d12::UAVTextureInfo{} });
        }
        return true;
    }

    case StorageBlockResourceType::structured_buffer_with_counter:
    case StorageBlockResourceType::append_structured_buffer:
    case StorageBlockResourceType::consume_structured_buffer:
    {
        if (m_next_counter_offset / D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT >= c_max_uav_with_counters_count)
        {
            logger().out(std::format("Unable to bind storage block to shader input '{}': atomic counters of the shader function are exhausted", name.string()),
                misc::LogMessageType::error);
            return false;
        }

        uint64_t counter_offset = m_next_counter_offset;
        m_next_counter_offset += D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
        d3d12::UAVBufferInfo info{
            .first_element = first_buffer_element,
            .num_elements = static_cast<uint32_t>((resource_desc.width - first_buffer_element * buffer_element_stride) / buffer_element_stride),
            .structure_byte_stride = buffer_element_stride,
            .counter_offset_in_bytes = counter_offset
        };
        table.table.p_heap->createUnorderedAccessViewDescriptor(offset, d3d12::UAVDescriptor{ storage_block, info, &m_uav_atomic_counters });
        return true;
    }

    case StorageBlockResourceType::structured_buffer:
    case StorageBlockResourceType::raw_buffer:
    {
        bool is_raw = p_info->resource_type == StorageBlockResourceType::raw_buffer;
        uint32_t stride = is_raw ? 4 : buffer_element_stride;
        d3d12::UAVBufferInfo info{
            .first_element = first_buffer_element,
            .num_elements = static_cast<uint32_t>((resource_desc.width - first_buffer_element * stride) / stride),
            .structure_byte_stride = is_raw ? 0 : stride,
            .counter_offset_in_bytes = 0
        };
        if (is_raw)
        {
            info.flags = d3d12::UnorderedAccessViewBufferInfoFlags::raw;
        }

        d3d12::UAVDescriptor uav_descriptor{ storage_block, info };
        if (is_raw)
        {
            uav_descriptor.overrideFormat(DXGI_FORMAT_R32_TYPELESS);
        }
        table.table.p_heap->createUnorderedAccessViewDescriptor(offset, uav_descriptor);
        return true;
    }

    default:
        LEXGINE_ASSUME;
    }

    return false;
}

bool ShaderFunction::bindSampler(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, FilterPack const& filter,
    math::Vector4f const& border_color, uint32_t element/* = 0*/) const
{
    BindingPlacement const* p_placement = findPlacement(table, name, element);
    if (!p_placement)
    {
        return false;
    }

    if (p_placement->kind != ShaderInputKind::sampler && p_placement->kind != ShaderInputKind::comparison_sampler)
    {
        logger().out(std::format("Unable to bind sampler to shader input '{}': the input is not a sampler", name.string()), misc::LogMessageType::error);
        return false;
    }
    if (filter.isComparison() != (p_placement->kind == ShaderInputKind::comparison_sampler))
    {
        logger().out(std::format("Unable to bind sampler to shader input '{}': comparison mode of the filter does not match the sampler declaration", name.string()),
            misc::LogMessageType::error);
        return false;
    }

    table.table.p_heap->createSamplerDescriptor(descriptorOffset(table, *p_placement, element), d3d12::SamplerDescriptor{ filter, border_color });
    return true;
}

std::optional<uint32_t> ShaderFunction::bindBindlessTexture(misc::HashedString const& name, d3d12::Resource const& texture, d3d12::DescriptorAllocator& allocator) const
{
    BindingPlacement const* p_placement = findBindlessPlacement(name);
    if (!p_placement)
    {
        return std::nullopt;
    }

    std::optional<d3d12::SRVDescriptor> view = makeTextureView(*p_placement, name, texture);
    if (!view)
    {
        return std::nullopt;
    }

    d3d12::DescriptorHeap& descriptor_heap = allocator.descriptorHeap();
    if (&descriptor_heap != &m_device.descriptorHeap(d3d12::DescriptorHeapType::cbv_srv_uav))
    {
        logger().out(std::format("Unable to bind bindless texture to shader input '{}': the allocator does not serve the resource descriptor heap of the device", name.string()),
            misc::LogMessageType::error);
        return std::nullopt;
    }

    d3d12::DescriptorTable slot = allocator.allocateDescriptorTable(1);
    descriptor_heap.createShaderResourceViewDescriptor(slot.offset, *view);
    return static_cast<uint32_t>(slot.offset);
}

std::optional<uint32_t> ShaderFunction::bindBindlessTexture(misc::HashedString const& name, d3d12::Resource const& texture, d3d12::BindlessDescriptorCache& cache) const
{
    BindingPlacement const* p_placement = findBindlessPlacement(name);
    if (!p_placement)
    {
        return std::nullopt;
    }

    std::optional<d3d12::SRVDescriptor> view = makeTextureView(*p_placement, name, texture);
    if (!view)
    {
        return std::nullopt;
    }

    return cache.getOrCreate(*view);
}

BindingPlacement const* ShaderFunction::findPlacement(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, uint32_t element) const
{
    std::unordered_map<misc::HashedString, BindingPlacement> const& bindings = bindingLayout().bindings;
    auto p = bindings.find(name);
    if (p == bindings.end())
    {
        logger().out(std::format("Shader function does not declare input '{}'", name.string()), misc::LogMessageType::error);
        return nullptr;
    }

    BindingPlacement const& placement = p->second;
    if (placement.is_unbounded)
    {
        logger().out(std::format("Shader input '{}' is an unbounded array and must be bound as a bindless resource", name.string()), misc::LogMessageType::error);
        return nullptr;
    }
    if (placement.table != table.id)
    {
        logger().out(std::format("Shader input '{}' does not belong to the given descriptor table", name.string()), misc::LogMessageType::error);
        return nullptr;
    }
    if (element >= placement.capacity)
    {
        logger().out(std::format("Unable to bind element {} of shader input '{}', which has only {} elements", element, name.string(), placement.capacity),
            misc::LogMessageType::error);
        return nullptr;
    }

    return &placement;
}

BindingPlacement const* ShaderFunction::findBindlessPlacement(misc::HashedString const& name) const
{
    std::unordered_map<misc::HashedString, BindingPlacement> const& bindings = bindingLayout().bindings;
    auto p = bindings.find(name);
    if (p == bindings.end())
    {
        logger().out(std::format("Shader function does not declare input '{}'", name.string()), misc::LogMessageType::error);
        return nullptr;
    }
    if (!p->second.is_unbounded || p->second.kind != ShaderInputKind::srv)
    {
        logger().out(std::format("Shader input '{}' is not an unbounded array of shader resource views", name.string()), misc::LogMessageType::error);
        return nullptr;
    }

    return &p->second;
}

std::optional<d3d12::SRVDescriptor> ShaderFunction::makeTextureView(BindingPlacement const& placement, misc::HashedString const& name, d3d12::Resource const& texture) const
{
    TextureShaderInputInfo const* p_info = std::get_if<TextureShaderInputInfo>(&placement.view);
    if (!p_info || p_info->resource_type != TextureResourceType::resource_with_dimension || p_info->is_buffer)
    {
        logger().out(std::format("Unable to bind texture to shader input '{}': the input is not a texture", name.string()), misc::LogMessageType::error);
        return std::nullopt;
    }

    if (p_info->is_array)
    {
        uint32_t array_size = static_cast<uint32_t>(texture.descriptor().depth);
        d3d12::SRVTextureArrayInfo info{};
        info.num_array_elements = p_info->is_cube ? array_size / 6 : array_size;
        return d3d12::SRVDescriptor{ texture, info, p_info->is_cube };
    }

    return d3d12::SRVDescriptor{ texture, d3d12::SRVTextureInfo{}, p_info->is_cube };
}

size_t ShaderFunction::descriptorOffset(ShaderFunctionDescriptorTable const& table, BindingPlacement const& placement, uint32_t element) const
{
    return table.table.offset + placement.first_descriptor + element;
}

}  // namespace lexgine::core::dx::dxcompilation
