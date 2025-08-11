#ifndef LEXGINE_CORE_DX_DXCOMPILATION_SHADER_STAGE_H
#define LEXGINE_CORE_DX_DXCOMPILATION_SHADER_STAGE_H

#include <dxcapi.h>
#include <d3d12shader.h>
#include <unordered_map>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/class_names.h"
#include "engine/core/dx/d3d12/common.h"
#include "engine/core/dx/d3d12/resource.h"
#include "engine/core/dx/d3d12/tasks/hlsl_compilation_task.h"
#include "engine/core/dx/d3d12/descriptor_table_builders.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/constant_buffer_reflection.h"
#include "engine/core/misc/hashed_string.h"

#include "lexgine_core_dx_dxcompilation_fwd.h"
#include "shader_function.h"

namespace lexgine::core::dx::dxcompilation {

template<typename T>
class ShaderStageAttorney;

enum class ShaderArgumentKind {
    input,
    output
};

struct ShaderArgumentInfoKey {
    std::string semantic_name;
    uint32_t semantic_index;

    bool operator==(ShaderArgumentInfoKey const&) const = default;
};

struct ShaderArgumentInfo {
    uint32_t register_index;
    DXGI_FORMAT format;
    ShaderArgumentKind kind;
};

struct BindingResult
{
    bool was_successful;
    size_t binding_register;

    operator bool() const { return was_successful; }
};

} // namespace lexgine::core::dx::dxcompilation

namespace std {

template <>
struct hash<lexgine::core::dx::dxcompilation::ShaderArgumentInfoKey> {
    size_t operator()(lexgine::core::dx::dxcompilation::ShaderArgumentInfoKey const& value) const
    {
        lexgine::core::misc::HashValue hash_value { value.semantic_name.data(), value.semantic_name.size() };
        hash_value.combine(&value.semantic_index, sizeof(value.semantic_index));
        return hash_value.part1() ^ hash_value.part2();
    }
};

} // namespace std

namespace lexgine::core::dx::dxcompilation 
{

class ShaderStage : public NamedEntity<lexgine::core::class_names::ShaderStage> {
    friend class ShaderStageAttorney<ShaderFunction>;
public:
    void build();

    unsigned int getInstructionCount();

    BindingResult bindTexture(misc::HashedString const& name, d3d12::Resource const& texture, uint32_t register_offset = 0);
    BindingResult bindTextureArray(misc::HashedString const& name, d3d12::Resource const& texture,
        uint32_t first_array_element, uint32_t array_element_count, uint32_t register_offset = 0);
    BindingResult bindTextureBuffer(misc::HashedString const& name, d3d12::Resource const& buffer_texture, uint64_t first_buffer_element, uint32_t buffer_element_stride, uint32_t register_offset = 0);
    
    BindingResult bindConstantBuffer(misc::HashedString const& name, d3d12::Resource const& buffer, uint32_t offset_from_buffer_start, uint32_t size_in_bytes, uint32_t register_offset = 0);
    
    BindingResult bindStorageBlock(misc::HashedString const& name, d3d12::Resource const& storage_block, uint64_t first_buffer_element, uint32_t buffer_element_stride, uint32_t register_offset = 0);
    
    BindingResult bindSampler(misc::HashedString const& name, FilterPack const& filter, math::Vector4f const& border_color, uint32_t register_offset = 0);

    lexgine::core::D3DDataBlob getShaderBytecode() const;
    d3d12::ConstantBufferReflection buildConstantBufferReflection(misc::HashedString const& constant_buffer_name) const;
    ShaderType getShaderType() const;
    ShaderModel getShaderModel() const;

    ShaderArgumentInfo const& getShaderArgumentInfo(ShaderArgumentKind kind, ShaderArgumentInfoKey const& key) const;
    std::unordered_map<ShaderArgumentInfoKey, ShaderArgumentInfo> const& getShaderArguments(ShaderArgumentKind kind) const;

    d3d12::tasks::HLSLCompilationTask* getTask() const { return m_shader_compilation_task_ptr; }
    bool isReady() const { return m_is_ready; }

private:
    enum class TextureResourceDataType {
        unorm = 1,
        snorm,
        sint,
        uint,
        float32,
        unknown,
        float64,
        continued
    };

    enum class TextureResourceType
    {
        texture1d,
        texture2d,
        texture3d,
        tbuffer,
        structured_buffer,
        raw_buffer
    };

    enum class StorageBlockResourceType
    {
        typed,
        structured_buffer,
        structured_buffer_with_counter,
        raw_buffer,
        append_structured_buffer,
        consume_structured_buffer
    };

    struct TextureShaderInputInfo
    {
        bool is_cube = false;
        bool is_ms = false;
        uint32_t ms_count;
        TextureResourceType resource_type;
        TextureResourceDataType data_type;
    };

    struct StorageBlockShaderInputInfo
    {
        StorageBlockResourceType resource_type;
    };


private:
    ShaderStage(Globals const& globals, d3d12::tasks::HLSLCompilationTask* p_shader_compilation_task, ShaderFunction* p_owning_shader_function);

    static uint32_t getDataTypeSize(TextureResourceDataType data_type);
    void collectShaderBindings();
    void collectShaderArguments(ShaderArgumentKind kind);

    BindingResult bindInternal(misc::HashedString const& name, size_t register_offset, 
        std::function<size_t(ShaderFunction::ShaderBindingPoint const&, d3d12::DescriptorAllocationManager*)> const& descriptor_creator);
    /*void fillDescriptorTableRanges(d3d12::RootEntryDescriptorTable& target_descriptor_table,
        d3d12::RootEntryDescriptorTable::RangeType range_type);*/

private:
    Globals const& m_globals;
    d3d12::tasks::HLSLCompilationTask* m_shader_compilation_task_ptr;
    ShaderFunction* m_owning_shader_function_ptr;
    bool m_is_ready{ false };

    std::string m_shader_name;
    Microsoft::WRL::ComPtr<IDxcUtils> m_dxc_utils;
    Microsoft::WRL::ComPtr<ID3D12ShaderReflection> m_shader_reflection;
    D3D12_SHADER_DESC m_shader_desc;

    std::unordered_map<ShaderArgumentInfoKey, ShaderArgumentInfo> m_shader_input_arguments;
    std::unordered_map<ShaderArgumentInfoKey, ShaderArgumentInfo> m_shader_output_arguments;

    std::unordered_map<misc::HashedString, ShaderFunction::ShaderBindingPoint> m_shader_resource_names_pool;
    std::unordered_map<ShaderFunction::ShaderBindingPoint, TextureShaderInputInfo, ShaderFunction::ShaderInputBindingPointHash> m_texture_shader_inputs;
    std::unordered_map<ShaderFunction::ShaderBindingPoint, StorageBlockShaderInputInfo, ShaderFunction::ShaderInputBindingPointHash> m_storage_block_inputs;
};

template<>
class ShaderStageAttorney<ShaderFunction>
{
    friend class ShaderFunction;

private:
    static std::unique_ptr<ShaderStage> createShaderStage(Globals const& globals, d3d12::tasks::HLSLCompilationTask* p_shader_compilation_task, ShaderFunction* p_owning_shader_function)
    {
        return std::unique_ptr<ShaderStage>{ new ShaderStage{ globals, p_shader_compilation_task, p_owning_shader_function } };
    }

    static std::unordered_map<misc::HashedString, ShaderFunction::ShaderBindingPoint> const& getShaderStageBindings(ShaderStage const* p_shader_stage)
    {
        return p_shader_stage->m_shader_resource_names_pool;
    }
};

}  // namespace lexgine::core::dx::dxcompilation

#endif