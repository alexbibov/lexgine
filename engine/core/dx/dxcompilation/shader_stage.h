#ifndef LEXGINE_CORE_DX_DXCOMPILATION_SHADER_STAGE_H
#define LEXGINE_CORE_DX_DXCOMPILATION_SHADER_STAGE_H

#include <dxcapi.h>
#include <d3d12shader.h>
#include <unordered_map>
#include <vector>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/d3d12/common.h"
#include "engine/core/dx/d3d12/resource.h"
#include "engine/core/dx/d3d12/caches/hlsl_shader_blob_cache.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/constant_buffer_reflection.h"
#include "engine/core/misc/hashed_string.h"
#include "engine/core/misc/hashes/xxhash128.h"

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

} // namespace lexgine::core::dx::dxcompilation

namespace std {

template <>
struct hash<lexgine::core::dx::dxcompilation::ShaderArgumentInfoKey> {
    size_t operator()(lexgine::core::dx::dxcompilation::ShaderArgumentInfoKey const& value) const
    {
        lexgine::core::misc::hashes::XXHash128 hash_value{};
        hash_value.create(value.semantic_name.data(), value.semantic_name.size());
        hash_value.combine(&value.semantic_index, sizeof(value.semantic_index));
        hash_value.finalize();
        return static_cast<size_t>(hash_value.fold());
    }
};

} // namespace std

namespace lexgine::core::dx::dxcompilation 
{

class ShaderStage : public NamedEntity<ShaderStage> {
    friend class ShaderStageAttorney<ShaderFunction>;
public:
    void build();

    unsigned int getInstructionCount();

    lexgine::core::dx::d3d12::D3DDataBlob getShaderBytecode() const;
    d3d12::ConstantBufferReflection buildConstantBufferReflection(misc::HashedString const& constant_buffer_name) const;
    ShaderType getShaderType() const;
    ShaderModel getShaderModel() const;

    ShaderArgumentInfo const& getShaderArgumentInfo(ShaderArgumentKind kind, ShaderArgumentInfoKey const& key) const;
    std::unordered_map<ShaderArgumentInfoKey, ShaderArgumentInfo> const& getShaderArguments(ShaderArgumentKind kind) const;

    d3d12::caches::HLSLShaderHandle getShaderHandle() const { return m_shader_handle; }
    bool isReady() const { return m_is_ready; }

private:
    ShaderStage(Globals const& globals, d3d12::caches::HLSLShaderHandle shader_handle, ShaderFunction* p_owning_shader_function);

    void collectShaderBindings();
    void collectShaderArguments(ShaderArgumentKind kind);
    std::vector<ReflectedDeclaration> reflectedDeclarations() const;

private:
    Globals const& m_globals;
    d3d12::caches::HLSLShaderBlobCache const& m_shader_blob_cache;
    d3d12::caches::HLSLShaderHandle m_shader_handle;
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
    std::unordered_map<ShaderFunction::ShaderBindingPoint, StorageBlockShaderInputInfo, ShaderFunction::ShaderInputBindingPointHash> m_storage_block_shader_inputs;
};

template<>
class ShaderStageAttorney<ShaderFunction>
{
    friend class ShaderFunction;

private:
    static std::unique_ptr<ShaderStage> createShaderStage(Globals const& globals, d3d12::caches::HLSLShaderHandle shader_handle, ShaderFunction* p_owning_shader_function)
    {
        return std::unique_ptr<ShaderStage>{ new ShaderStage{ globals, shader_handle, p_owning_shader_function } };
    }

    static std::vector<ReflectedDeclaration> getShaderStageDeclarations(ShaderStage const* p_shader_stage)
    {
        return p_shader_stage->reflectedDeclarations();
    }
};

}  // namespace lexgine::core::dx::dxcompilation

#endif
