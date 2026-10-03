#ifndef LEXGINE_CORE_DX_DXCOMPILATION_SHADER_FUNCTION_H
#define LEXGINE_CORE_DX_DXCOMPILATION_SHADER_FUNCTION_H

#include <array>
#include <bitset>
#include <unordered_set>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/root_signature.h"
#include "engine/core/dx/d3d12/caches/hlsl_shader_blob_cache.h"
#include "engine/core/dx/d3d12/caches/root_signature_blob_cache.h"
#include "engine/core/dx/d3d12/descriptor_allocation_manager.h"
#include "engine/core/misc/hashes/xxhash128.h"
#include "lexgine_core_dx_dxcompilation_fwd.h"
#include "common.h"
#include "shader_binding_layout.h"


namespace lexgine::core::dx::dxcompilation {

template<typename T>
class ShaderFunctionAttorney;

enum class ShaderFunctionConstantBufferRootIds
{
    scene_uniforms = 0,
    material_uniforms,
    instanced_material_uniforms,
    object_uniforms,
    count
};

BEGIN_FLAGS_DECLARATION(ShaderFunctionRootUniformBuffers)
FLAG(None, 0)
FLAG(SceneUniforms, 1 << static_cast<int>(ShaderFunctionConstantBufferRootIds::scene_uniforms))
FLAG(MaterialUniforms, 1 << static_cast<int>(ShaderFunctionConstantBufferRootIds::material_uniforms))
FLAG(ModelUniforms, 1 << static_cast<int>(ShaderFunctionConstantBufferRootIds::instanced_material_uniforms))
FLAG(All, static_cast<int>(SceneUniforms) | static_cast<int>(MaterialUniforms) | static_cast<int>(ModelUniforms))
END_FLAGS_DECLARATION(ShaderFunctionRootUniformBuffers);

enum class ShaderResourceBindingFrequency
{
    persistent,
    per_frame
};

class ShaderFunction : public NamedEntity<ShaderFunction>, public ProvidesGlobals
{
    friend class ShaderFunctionAttorney<ShaderStage>;
public:
    constexpr static uint32_t c_reserved_constant_buffer_space_id = c_root_constant_buffer_register_space;

public:
    using ShaderInputKind = dxcompilation::ShaderInputKind;
    using ShaderBindingPoint = dxcompilation::ShaderBindingPoint;
    using ShaderInputBindingPointHash = dxcompilation::ShaderInputBindingPointHash;

public:
    ShaderFunction(Globals& globals, ShaderFunctionRootUniformBuffers const& flags);
    ShaderFunction(ShaderFunction const&) = delete;
    ~ShaderFunction();

    ShaderStage* getShaderStage(ShaderType shader_type) const { return m_shader_stages[static_cast<size_t>(shader_type)].get(); }
    ShaderStage* createShaderStage(d3d12::caches::HLSLShaderHandle shader_handle);

    void collectInputResourceBindings();
    d3d12::caches::RootSignatureHandle buildInputResourceBindings();

    uint32_t occupiedRootSignatureSlotsCount() const { return m_occupied_rs_slots; }

    void bindRootConstantBuffer(core::dx::d3d12::CommandList& command_list, 
        ShaderFunctionConstantBufferRootIds id, 
        uint64_t gpu_virtual_address) const;

    bool assignResourceDescriptors(ShaderInputKind resource_kind, uint32_t resource_space_id, const core::dx::d3d12::DescriptorAllocationManager& allocation_manager);
    bool bindResourceDescriptors(core::dx::d3d12::CommandList& command_list, ShaderInputKind kind, uint32_t space_id);

private:
    struct ShaderInputDesc
    {
        ShaderResourceBindingFrequency binding_frequency;
        std::bitset<static_cast<size_t>(ShaderType::count)> shader_stage_presence;
    };

    struct DescriptorTableKey
    {
        ShaderInputKind kind;
        uint32_t space_id;

        bool operator==(DescriptorTableKey const&) const = default;
    };

    struct DescriptorTableKeyHash
    {
        size_t operator()(DescriptorTableKey const& value) const
        {
            misc::hashes::XXHash128 hash_value{};
            hash_value.create(&value, sizeof(DescriptorTableKey));
            hash_value.finalize();
            return static_cast<size_t>(hash_value.fold());
        }
    };

private:
    static const size_t c_max_uav_with_counters_count = 32;

private:
    d3d12::Device& m_device;
    ShaderFunctionRootUniformBuffers m_flags;

    std::array<std::unique_ptr<ShaderStage>, static_cast<size_t>(ShaderType::count)> m_shader_stages;

    std::unordered_map<ShaderBindingPoint, ShaderInputDesc, ShaderInputBindingPointHash> m_shader_inputs;

    std::unordered_map<DescriptorTableKey, d3d12::RootEntryDescriptorTable, DescriptorTableKeyHash> m_assumed_descriptor_tables{};
    std::unordered_set<uint32_t> m_assumed_register_spaces{};

    std::unordered_map<ShaderFunctionConstantBufferRootIds, uint32_t> m_root_uniforms_to_rs_slots_mapping;
    std::unordered_map<DescriptorTableKey, uint32_t, DescriptorTableKeyHash> m_descriptor_table_keys_to_rs_slots_mapping;
    std::unordered_map<DescriptorTableKey, size_t, DescriptorTableKeyHash> m_descriptor_table_capacities;
    std::unordered_map<DescriptorTableKey, core::dx::d3d12::DescriptorAllocationManager, DescriptorTableKeyHash> m_descriptor_table_allocators;

    uint32_t m_occupied_rs_slots{ 0 };

    uint64_t m_next_counter_offset { 0 };
    d3d12::Resource m_uav_atomic_counters;

    d3d12::caches::RootSignatureHandle m_root_signature_handle { nullptr };
    bool m_shader_function_stale{ true };
};


template<>
class ShaderFunctionAttorney<ShaderStage>
{
    friend class ShaderStage;

    struct AtomicCounterDesc
    {
        d3d12::Resource& atomic_counter_resource;
        uint64_t offset;
    };

    static core::dx::d3d12::DescriptorAllocationManager* getDescriptorAllocationManager(ShaderFunction& shader_function, ShaderFunction::ShaderInputKind kind, uint32_t space_id)
    {
        ShaderFunction::DescriptorTableKey key{ .kind = kind, .space_id = space_id };
        if (shader_function.m_descriptor_table_allocators.count(key) == 0)
        {
            return nullptr;
        }

        return &shader_function.m_descriptor_table_allocators.at(key);
    }

    static AtomicCounterDesc allocateAtomicCounter(ShaderFunction& shader_function)
    {
        assert(shader_function.m_next_counter_offset / D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT < ShaderFunction::c_max_uav_with_counters_count);
        uint64_t counter_offset = shader_function.m_next_counter_offset;
        shader_function.m_next_counter_offset += D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
        return { shader_function.m_uav_atomic_counters, counter_offset };
    }
};


}

#endif
