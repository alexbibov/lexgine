#ifndef LEXGINE_CORE_DX_DXCOMPILATION_SHADER_FUNCTION_H
#define LEXGINE_CORE_DX_DXCOMPILATION_SHADER_FUNCTION_H

#include <array>
#include <bitset>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/root_signature.h"
#include "engine/core/dx/d3d12/static_descriptor_allocation_manager.h"

#include "engine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"

#include "lexgine_core_dx_dxcompilation_fwd.h"
#include "common.h"


namespace lexgine::core::dx::dxcompilation {

template<typename T>
class ShaderFunctionAttorney;

enum class ShaderFunctionConstantBufferRootIds
{
    scene_uniforms = 0,
    // model_uniforms,
    // material_uniforms,
    count
};

class ShaderFunction : public NamedEntity<class_names::ShaderFunction>
{
    friend class ShaderFunctionAttorney<ShaderStage>;
public:
    constexpr static uint32_t c_reserved_constant_buffer_space_id = 100;

    enum class ShaderInputKind {
        srv, uav, cbv, sampler
    };

    struct ShaderBindingPoint {
        ShaderInputKind kind;
        uint32_t first_register;
        uint32_t register_count;
        uint32_t register_space;

        bool operator==(ShaderBindingPoint const& other) const = default;
    };

    struct ShaderInputBindingPointHash {
        size_t operator()(ShaderBindingPoint const& binding_point) const
        {
            return static_cast<uint64_t>(binding_point.kind)
                | (static_cast<uint64_t>(binding_point.first_register) << 2)
                | (static_cast<uint64_t>(binding_point.register_count) << 18)
                | (static_cast<uint64_t>(binding_point.register_space) << 34);
        }
    };

public:
    ShaderFunction(Globals& globals, uint32_t descriptor_heap_page_id, std::string const& name);
    std::string const& name() const { return m_name; }
    ShaderStage* getShaderStage(ShaderType shader_type) const { return m_shader_stages[static_cast<size_t>(shader_type)].get(); }
    ShaderStage* createShaderStage(d3d12::tasks::HLSLCompilationTask const* p_shader_compilation_task);
    void prepare(bool prepare_asynchronously);

    d3d12::tasks::RootSignatureCompilationTask* getBindingSignature() const { return m_p_root_signature_compilation_task; }

private:
    struct ShaderInputDesc
    {
        std::bitset<static_cast<size_t>(ShaderType::count)> shader_stage_presence;
        int bound_resource_index{ -1 };
    };

private:
    static const size_t c_max_uav_with_counters_count = 128;

private:
    Globals& m_globals;
    d3d12::Device& m_device;
    d3d12::StaticDescriptorAllocationManager& m_static_cbv_srv_uav_descriptor_allocator;
    d3d12::StaticDescriptorAllocationManager& m_static_sampler_descriptor_allocator;
    uint32_t m_descriptor_heap_page_id;
    std::string m_name;

    std::array<std::unique_ptr<ShaderStage>, static_cast<size_t>(ShaderType::count)> m_shader_stages;

    std::unordered_map<ShaderBindingPoint, ShaderInputDesc, ShaderInputBindingPointHash> m_shader_inputs;

    std::vector<d3d12::StaticDescriptorAllocationManager::UPointer> m_bound_cbv_resources;
    std::vector<d3d12::StaticDescriptorAllocationManager::UPointer> m_bound_srv_resources;
    std::vector<d3d12::StaticDescriptorAllocationManager::UPointer> m_bound_uav_resources;
    std::vector<d3d12::StaticDescriptorAllocationManager::UPointer> m_bound_sampler_resources;

    uint64_t m_next_counter_offset { 0 };
    d3d12::Resource m_uav_atomic_counters;

    d3d12::tasks::RootSignatureCompilationTask* m_p_root_signature_compilation_task{ nullptr };
    bool m_shader_function_stale{ true };
};

template<>
class ShaderFunctionAttorney<ShaderStage>
{
    friend class ShaderStage;

private:
    static void bindResourceToShaderFunctionInput(ShaderFunction& target_shader_function,
        ShaderType binging_shader_stage_type,
        ShaderFunction::ShaderBindingPoint const& binding_point,
        d3d12::StaticDescriptorAllocationManager::UPointer const& resource_descriptor_pointer)
    {
        ShaderFunction::ShaderInputDesc& shader_input_desc = target_shader_function.m_shader_inputs[binding_point];
        
        std::vector<d3d12::StaticDescriptorAllocationManager::UPointer>* p_resources{};
        switch (binding_point.kind)
        {
        case ShaderFunction::ShaderInputKind::srv:
            p_resources = &target_shader_function.m_bound_srv_resources;
            break;
        case ShaderFunction::ShaderInputKind::cbv:
            p_resources = &target_shader_function.m_bound_cbv_resources;
            break;
        case ShaderFunction::ShaderInputKind::uav:
            p_resources = &target_shader_function.m_bound_uav_resources;
            break;
        case ShaderFunction::ShaderInputKind::sampler:
            p_resources = &target_shader_function.m_bound_sampler_resources;
            break;
        }
        
        if (shader_input_desc.bound_resource_index == -1)
        {
            shader_input_desc.bound_resource_index = static_cast<int>(p_resources->size());
            p_resources->push_back(resource_descriptor_pointer);
        }
        else
        {
            p_resources->data()[shader_input_desc.bound_resource_index] = resource_descriptor_pointer;
        }

        shader_input_desc.shader_stage_presence.set(static_cast<size_t>(binging_shader_stage_type));
        target_shader_function.m_shader_function_stale = true;
    }

    static d3d12::StaticDescriptorAllocationManager& getStaticCbvSrvUavDescriptorAllocator(ShaderFunction const& target_shader_function)
    {
        return target_shader_function.m_static_cbv_srv_uav_descriptor_allocator;
    }

    static d3d12::StaticDescriptorAllocationManager& getStaticSamplerDescriptorAllocator(ShaderFunction const& target_shader_function)
    {
        return target_shader_function.m_static_sampler_descriptor_allocator;
    }

    static d3d12::Resource* getUavAtomicCountersResource(ShaderFunction& target_shader_function)
    {
        return &target_shader_function.m_uav_atomic_counters;
    }

    static uint64_t stepUavCounterOffset(ShaderFunction& target_shader_function)
    {
        uint64_t old_value = target_shader_function.m_next_counter_offset;
        target_shader_function.m_next_counter_offset += D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
        assert(target_shader_function.m_next_counter_offset <= D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT * ShaderFunction::c_max_uav_with_counters_count);
        return old_value;
    }
};

}

#endif