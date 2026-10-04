#ifndef LEXGINE_CORE_DX_DXCOMPILATION_SHADER_FUNCTION_H
#define LEXGINE_CORE_DX_DXCOMPILATION_SHADER_FUNCTION_H

#include <array>
#include <optional>
#include <vector>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/entity.h"
#include "engine/core/filter.h"
#include "engine/core/math/vector_types.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/root_signature.h"
#include "engine/core/dx/d3d12/resource.h"
#include "engine/core/dx/d3d12/srv_descriptor.h"
#include "engine/core/dx/d3d12/caches/hlsl_shader_blob_cache.h"
#include "engine/core/dx/d3d12/caches/root_signature_blob_cache.h"
#include "engine/core/misc/hashed_string.h"
#include "lexgine_core_dx_dxcompilation_fwd.h"
#include "common.h"
#include "shader_binding_layout.h"


namespace lexgine::core::dx::dxcompilation {

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

//! Descriptor table allocated for one of the descriptor table layouts compiled by a shader function
struct ShaderFunctionDescriptorTable
{
    static constexpr DescriptorTableId c_invalid_id = static_cast<DescriptorTableId>(-1);

    DescriptorTableId id = c_invalid_id;
    d3d12::DescriptorTable table{};
};

class ShaderFunction : public NamedEntity<ShaderFunction>, public ProvidesGlobals
{
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

    void collectInputResourceBindings();    //! builds reflection of all shader stages and gathers their input declarations

    /*! compiles binding layout from the gathered input declarations and creates root signature compilation contract for it.
     Throws if the declarations cannot be organized into a valid binding layout
    */
    d3d12::caches::RootSignatureHandle buildInputResourceBindings();

    uint32_t occupiedRootSignatureSlotsCount() const { return m_occupied_rs_slots; }

    CompiledBindingLayout const& bindingLayout() const;    //! returns binding layout compiled by buildInputResourceBindings()

    //! returns identifier of the descriptor table of the given binding domain residing in descriptor heap of the given type, if the layout contains such table
    std::optional<DescriptorTableId> findDescriptorTable(BindingDomain domain, d3d12::DescriptorHeapType heap_type) const;

    //! allocates descriptors for the descriptor table identified by @param id using @param allocator
    ShaderFunctionDescriptorTable createDescriptorTable(DescriptorTableId id, d3d12::DescriptorAllocator& allocator) const;

    void bindRootConstantBuffer(d3d12::CommandList& command_list, ShaderFunctionConstantBufferRootIds id, uint64_t gpu_virtual_address) const;
    void setDescriptorTable(d3d12::CommandList& command_list, ShaderFunctionDescriptorTable const& table) const;    //! binds @param table to its root signature slot
    void setBindlessDescriptorTables(d3d12::CommandList& command_list) const;    //! binds bindless descriptor tables, which span the whole descriptor heaps

    /*! The following functions write descriptor of the named shader input into @param table at array element @param element.
     They return 'false' and log the reason if the input is not found in the table or the resource does not meet the view requirements of the input
    */
    bool bindTexture(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, d3d12::Resource const& texture, uint32_t element = 0) const;
    bool bindTextureArray(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, d3d12::Resource const& texture,
        uint32_t first_array_element, uint32_t array_element_count, uint32_t element = 0) const;
    bool bindTextureBuffer(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, d3d12::Resource const& buffer,
        uint64_t first_buffer_element, uint32_t buffer_element_stride, uint32_t element = 0) const;
    bool bindConstantBuffer(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, d3d12::Resource const& buffer,
        uint32_t offset_from_buffer_start, uint32_t size_in_bytes, uint32_t element = 0) const;
    bool bindStorageBlock(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, d3d12::Resource const& storage_block,
        uint64_t first_buffer_element, uint32_t buffer_element_stride, uint32_t element = 0);
    bool bindSampler(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, FilterPack const& filter,
        math::Vector4f const& border_color, uint32_t element = 0) const;

    /*! Writes shader resource view of @param texture for the named unbounded array input and returns index of the descriptor,
     which the shader uses to access the texture through the array. The descriptor is reserved using @param allocator.
     Returns an empty optional and logs the reason if the texture cannot be bound
    */
    std::optional<uint32_t> bindBindlessTexture(misc::HashedString const& name, d3d12::Resource const& texture, d3d12::DescriptorAllocator& allocator) const;

    //! Same as above, but the descriptor is obtained from @param cache, which reuses descriptors of identical views
    std::optional<uint32_t> bindBindlessTexture(misc::HashedString const& name, d3d12::Resource const& texture, d3d12::BindlessDescriptorCache& cache) const;

private:
    static const size_t c_max_uav_with_counters_count = 32;

private:
    BindingPlacement const* findPlacement(ShaderFunctionDescriptorTable const& table, misc::HashedString const& name, uint32_t element) const;
    BindingPlacement const* findBindlessPlacement(misc::HashedString const& name) const;
    std::optional<d3d12::SRVDescriptor> makeTextureView(BindingPlacement const& placement, misc::HashedString const& name, d3d12::Resource const& texture) const;
    size_t descriptorOffset(ShaderFunctionDescriptorTable const& table, BindingPlacement const& placement, uint32_t element) const;

private:
    d3d12::Device& m_device;
    ShaderFunctionRootUniformBuffers m_flags;

    std::array<std::unique_ptr<ShaderStage>, static_cast<size_t>(ShaderType::count)> m_shader_stages;

    std::vector<ReflectedDeclaration> m_reflected_declarations;
    std::optional<CompiledBindingLayout> m_binding_layout;
    std::unordered_map<ShaderFunctionConstantBufferRootIds, uint32_t> m_root_uniforms_to_rs_slots_mapping;
    uint32_t m_occupied_rs_slots{ 0 };

    uint64_t m_next_counter_offset { 0 };
    d3d12::Resource m_uav_atomic_counters;

    d3d12::caches::RootSignatureHandle m_root_signature_handle { nullptr };
    bool m_shader_function_stale{ true };
};

}

#endif
