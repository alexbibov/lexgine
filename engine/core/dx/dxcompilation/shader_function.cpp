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

namespace
{

char const* shaderInputKindToString(ShaderFunction::ShaderInputKind kind)
{
    switch (kind)
    {
    case ShaderFunction::ShaderInputKind::srv:
        return "srv";
    case ShaderFunction::ShaderInputKind::uav:
        return "uav";
    case ShaderFunction::ShaderInputKind::cbv:
        return "cbv";
    case ShaderFunction::ShaderInputKind::sampler:
        return "sampler";
    default:
        return "";
    }
}

}

ShaderFunction::ShaderFunction(Globals& globals, ShaderFunctionRootUniformBuffers const& flags/* = ShaderFunctionRootUniformBuffers::base_values::None*/)
    : ProvidesGlobals { globals }
    , m_device { *globals.get<d3d12::Device>() }
    , m_flags{ flags }
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

ShaderFunction::~ShaderFunction() = default;

ShaderStage* ShaderFunction::createShaderStage(d3d12::tasks::HLSLCompilationTask* p_shader_compilation_tasks)
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



d3d12::tasks::RootSignatureCompilationTask* ShaderFunction::buildBindingSignature()
{
    buildInternal();
    return m_root_signature_compilation_task_ptr;
}

void ShaderFunction::bindRootConstantBuffer(core::dx::d3d12::CommandList& command_list, 
    ShaderFunctionConstantBufferRootIds id, 
    uint64_t gpu_virtual_address)
{
    command_list.setRootConstantBufferView(m_root_uniforms_to_rs_slots_mapping[id], gpu_virtual_address);
}

bool ShaderFunction::assignResourceDescriptors(ShaderInputKind resource_kind, uint32_t resource_space_id, const core::dx::d3d12::DescriptorAllocationManager& allocation_manager)
{
    DescriptorTableKey key{ .kind = resource_kind, .space_id = resource_space_id };
    
    if (m_descriptor_table_keys_to_rs_slots_mapping.count(key) == 0)
    {
        core::misc::Log::retrieve()->out(
            std::string{ "WARNING: unable to bind descriptor allocation manager for " }
            + shaderInputKindToString(resource_kind)
            + " descriptor table located in space#"
            + std::to_string(resource_space_id)
            + ": shader function does not declare inputs with requested properties",
            core::misc::LogMessageType::exclamation);
        return false;
    }

    if (!m_descriptor_table_allocators.count(key))
    {
        m_descriptor_table_allocators.insert(std::make_pair(key, allocation_manager));
    }
    else
    {
        m_descriptor_table_allocators.at(key) = allocation_manager;
    }

    bool buildResult = m_descriptor_table_allocators.at(key).build(m_descriptor_table_capacities[key]);
    buildResult;
    assert(buildResult);

    return true;
}

bool ShaderFunction::bindResourceDescriptors(core::dx::d3d12::CommandList& command_list, ShaderInputKind kind, uint32_t space_id)
{
    DescriptorTableKey key{ .kind = kind, .space_id = space_id };
    if (m_descriptor_table_allocators.count(key) == 0)
    {
        core::misc::Log::retrieve()->out(
            std::string{ "WARNING: unable to update descriptor table binding for " }
            + shaderInputKindToString(kind)
            + " descriptor table located in space#"
            + std::to_string(space_id)
            + ": shader function does not declare inputs with requested properties",
            core::misc::LogMessageType::exclamation);
        return false;
    }

    uint32_t root_signature_slot = m_descriptor_table_keys_to_rs_slots_mapping.at(key);
    core::dx::d3d12::DescriptorAllocationManager const& descriptor_allocation_manager = m_descriptor_table_allocators.at(key);
    command_list.setRootDescriptorTable(root_signature_slot, descriptor_allocation_manager.getDescriptorTable().gpu_pointer);
    
    return true;
}

void ShaderFunction::buildInternal()
{
    if (!m_shader_function_stale) {
        return;
    }

    d3d12::RootSignature rs {};
    m_descriptor_table_keys_to_rs_slots_mapping.clear();
    
    for (int shader_stage_id = 0; shader_stage_id < static_cast<int>(ShaderType::count); ++shader_stage_id)
    {
        if (ShaderStage* p_shader_stage = m_shader_stages[shader_stage_id].get())
        {
            p_shader_stage->build();
            for (auto const [_, shader_binding_point] : ShaderStageAttorney<ShaderFunction>::getShaderStageBindings(p_shader_stage))
            {
                auto [p, __] = m_shader_inputs.insert(
                    std::make_pair(shader_binding_point, ShaderInputDesc{})
                );
                p->second.shader_stage_presence.set(static_cast<size_t>(shader_stage_id));
            }
        }
    }
    
    for (auto const [binding_point, shader_input_desc] : m_shader_inputs)
    {
        if (binding_point.kind == ShaderInputKind::cbv 
            && binding_point.register_space == c_reserved_constant_buffer_space_id)
        {
            continue;
        }

        DescriptorTableKey key{ .kind = binding_point.kind, .space_id = binding_point.register_space };
        m_descriptor_table_capacities[key] = (std::max)(m_descriptor_table_capacities[key], static_cast<size_t>(binding_point.first_register) + binding_point.register_count);
        if (m_assumed_descriptor_tables.count(key))
        {
            continue;
        }
        m_assumed_register_spaces.insert(key.space_id);

        d3d12::RootEntryDescriptorTable& target_descriptor_table = m_assumed_descriptor_tables[key];
        d3d12::RootEntryDescriptorTable::RangeType range_type = static_cast<d3d12::RootEntryDescriptorTable::RangeType>(binding_point.kind);
        d3d12::RootEntryDescriptorTable::Range range{ range_type, binding_point.register_count, binding_point.first_register, binding_point.register_space, binding_point.first_register };
        target_descriptor_table.addRange(range);
    }

    m_occupied_rs_slots = 0;
    for (int id = static_cast<int>(ShaderFunctionConstantBufferRootIds::scene_uniforms); id < static_cast<int>(ShaderFunctionConstantBufferRootIds::count); ++id) {
        if (m_flags.isSet(static_cast<ShaderFunctionRootUniformBuffers::base_values>(1 << id))) 
        {
            d3d12::RootEntryCBVDescriptor cbv_descriptor { static_cast<uint32_t>(id), c_reserved_constant_buffer_space_id };
            m_root_uniforms_to_rs_slots_mapping[static_cast<ShaderFunctionConstantBufferRootIds>(id)] = m_occupied_rs_slots;
            rs.addParameter(m_occupied_rs_slots++, cbv_descriptor);
        }
    }
   
    for (uint32_t register_space_id : m_assumed_register_spaces)
    {
        for (int kind_id = 0; kind_id < static_cast<int>(ShaderInputKind::count); ++kind_id)
        {
            ShaderInputKind kind = static_cast<ShaderInputKind>(kind_id);
            DescriptorTableKey key{ .kind = kind, .space_id = register_space_id };
            if (m_assumed_descriptor_tables.contains(key))
            {
                assert(m_descriptor_table_keys_to_rs_slots_mapping.count(key) == 0);
                m_descriptor_table_keys_to_rs_slots_mapping[key] = m_occupied_rs_slots;

                rs.addParameter(m_occupied_rs_slots++, m_assumed_descriptor_tables[key]);
            }
        }
    }

    d3d12::task_caches::RootSignatureCompilationTaskCache* rs_compilation_task_cache = m_globals.get<d3d12::task_caches::RootSignatureCompilationTaskCache>();
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

    m_root_signature_compilation_task_ptr = rs_compilation_task_cache->findOrCreateTask(m_globals, d3d12::task_caches::RootSignatureCompilationTaskCache::VersionedRootSignature { std::move(rs) }, rs_flags, getStringName() + "_root_signature", 0);
    m_shader_function_stale = false;
}

}  // namespace lexgine::core::dx::dxcompilation