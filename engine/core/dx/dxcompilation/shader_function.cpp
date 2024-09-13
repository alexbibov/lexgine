#include <unordered_set>

#include "engine/core/exception.h"
#include "engine/core/globals.h"
#include "engine/core/misc/log.h"
#include "engine/core/misc/hash_value.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/d3d12/descriptor_heap.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/command_list.h"
#include "engine/core/dx/d3d12/tasks/root_signature_compilation_task.h"
#include "shader_function.h"
#include "shader_stage.h"


namespace lexgine::core::dx::dxcompilation {

ShaderFunction::ShaderFunction(Globals& globals, uint32_t descriptor_heap_page_id, std::string const& name)
    : m_globals { globals }
    , m_device { *globals.get<d3d12::Device>() }
    , m_static_cbv_srv_uav_descriptor_allocator{ globals.get<d3d12::DxResourceFactory>()->getStaticAllocationManagerForDescriptorHeap(m_device, d3d12::DescriptorHeapType::cbv_srv_uav, descriptor_heap_page_id) }
    , m_static_sampler_descriptor_allocator{ globals.get<d3d12::DxResourceFactory>()->getStaticAllocationManagerForDescriptorHeap(m_device, d3d12::DescriptorHeapType::sampler, descriptor_heap_page_id) }
    , m_descriptor_heap_page_id{ descriptor_heap_page_id }
    , m_name{ name }
{
    // Create buffer for atomic counters
    d3d12::ResourceDescriptor counter_resource_desc = d3d12::ResourceDescriptor::CreateBuffer(D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT * c_max_uav_with_counters_count, d3d12::ResourceFlags::base_values::none);
    m_uav_atomic_counters = d3d12::CommittedResource {
        m_device,
        d3d12::ResourceState::base_values::unordered_access,
        misc::Optional<d3d12::ResourceOptimizedClearValue> {},
        counter_resource_desc,
        d3d12::AbstractHeapType::_default,
        d3d12::HeapCreationFlags::base_values::allow_all
    };
}

ShaderStage* ShaderFunction::createShaderStage(d3d12::tasks::HLSLCompilationTask const* p_shader_compilation_tasks)
{
   
    std::unique_ptr<ShaderStage> new_shader_stage = ShaderStageAttorney<ShaderFunction>::createShaderStage(m_globals, p_shader_compilation_tasks, this);
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

void ShaderFunction::prepare(bool prepare_asynchronously)
{
    if (!m_shader_function_stale) 
    {
        return; 
    }

    d3d12::RootEntryDescriptorTable cbv_srv_uav_binding_table{}, sampler_binding_table{};
    d3d12::RootSignature rs{};

    for (auto const& e : m_shader_inputs)
    {
        ShaderBindingPoint const& binding_point = e.first;
        int resource_index = e.second.bound_resource_index;
        if (resource_index >= 0)
        {
            d3d12::RootEntryDescriptorTable* p_target_binding_table{ nullptr };
            uint32_t descriptor_offset{};
            switch (binding_point.kind)
            {
            case ShaderInputKind::cbv:
                p_target_binding_table = &cbv_srv_uav_binding_table;
                descriptor_offset = m_bound_cbv_resources[resource_index].descriptor_offset;
                break;

            case ShaderInputKind::srv:
                p_target_binding_table = &cbv_srv_uav_binding_table;
                descriptor_offset = m_bound_srv_resources[resource_index].descriptor_offset;
                break;

            case ShaderInputKind::uav:
                p_target_binding_table = &cbv_srv_uav_binding_table;
                descriptor_offset = m_bound_uav_resources[resource_index].descriptor_offset;
                break;

            case ShaderInputKind::sampler:
                p_target_binding_table = &sampler_binding_table;
                descriptor_offset = m_bound_sampler_resources[resource_index].descriptor_offset;
                break;

            default:
                LEXGINE_ASSUME;
            }

            d3d12::RootEntryDescriptorTable::RangeType range_type = static_cast<d3d12::RootEntryDescriptorTable::RangeType>(binding_point.kind);
            d3d12::RootEntryDescriptorTable::Range range{ range_type, binding_point.register_count, binding_point.first_register, binding_point.register_space, descriptor_offset };
            p_target_binding_table->addRange(range);
        }
    }

    for (int id = static_cast<int>(ShaderFunctionConstantBufferRootIds::scene_uniforms); id < static_cast<int>(ShaderFunctionConstantBufferRootIds::count); ++id) {
        d3d12::RootEntryCBVDescriptor cbv_descriptor { static_cast<uint32_t>(id), c_reserved_constant_buffer_space_id };
        rs.addParameter(id, cbv_descriptor);
    }
    if (!cbv_srv_uav_binding_table.empty())
    {
        rs.addParameter(static_cast<uint32_t>(ShaderFunctionConstantBufferRootIds::count), cbv_srv_uav_binding_table);
    }
    if (!sampler_binding_table.empty())
    {
        rs.addParameter(static_cast<uint32_t>(ShaderFunctionConstantBufferRootIds::count) + 1, sampler_binding_table);
    }

    d3d12::task_caches::RootSignatureCompilationTaskCache* rs_compilation_task_cache = m_globals.get<d3d12::task_caches::RootSignatureCompilationTaskCache>();
    d3d12::RootSignatureFlags rs_flags = d3d12::RootSignatureFlags::base_values::deny_vertex_shader
        | d3d12::RootSignatureFlags::base_values::deny_hull_shader
        | d3d12::RootSignatureFlags::base_values::deny_domain_shader
        | d3d12::RootSignatureFlags::base_values::deny_geometry_shader
        | d3d12::RootSignatureFlags::base_values::deny_pixel_shader;

    for (int shader_type_id = 0; shader_type_id < static_cast<int>(ShaderType::count); ++shader_type_id)
    {
        ShaderType shader_type = static_cast<ShaderType>(shader_type_id);
        if (!m_shader_stages[shader_type_id])
        {
            continue;
        }

        switch (shader_type)
        {
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

    m_p_root_signature_compilation_task = rs_compilation_task_cache->findOrCreateTask(m_globals, d3d12::task_caches::RootSignatureCompilationTaskCache::VersionedRootSignature{ std::move(rs) }, rs_flags, m_name + "_root_signature", m_descriptor_heap_page_id);
    if (!prepare_asynchronously)
    {
        m_p_root_signature_compilation_task->execute(0);
    }

    m_shader_function_stale = false;
}

}  // namespace lexgine::core::dx::dxcompilation