#ifndef LEXGINE_CORE_DX_DXCOMPILATION_SHADER_BINDING_LAYOUT_H
#define LEXGINE_CORE_DX_DXCOMPILATION_SHADER_BINDING_LAYOUT_H

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "engine/core/dx/d3d12/descriptor_heap.h"
#include "engine/core/dx/d3d12/root_signature.h"
#include "engine/core/misc/hashed_string.h"
#include "engine/core/misc/hashes/xxhash128.h"
#include "common.h"

namespace lexgine::core::dx::dxcompilation {

//! Register space reserved for constant buffers bound directly to root signature parameters
constexpr uint32_t c_root_constant_buffer_register_space = 100;

//! Kinds of shader inputs exposed by shader reflection
enum class ShaderInputKind
{
    srv,
    uav,
    cbv,
    sampler,
    comparison_sampler,
    count
};

//! Register range occupied by a reflected shader input
struct ShaderBindingPoint
{
    ShaderInputKind kind;
    uint32_t first_register;
    uint32_t register_count;    //!< number of registers occupied by the input; equals 0 for unbounded arrays
    uint32_t register_space;
    bool is_unbounded;

    bool operator==(ShaderBindingPoint const&) const = default;
};

struct ShaderInputBindingPointHash
{
    size_t operator()(ShaderBindingPoint const& binding_point) const
    {
        uint32_t const fields[] = {
            static_cast<uint32_t>(binding_point.kind),
            binding_point.first_register,
            binding_point.register_count,
            binding_point.register_space,
            static_cast<uint32_t>(binding_point.is_unbounded)
        };

        misc::hashes::XXHash128 hash_value{};
        hash_value.create(fields, sizeof(fields));
        hash_value.finalize();
        return static_cast<size_t>(hash_value.fold());
    }
};

//! Element data type of a typed resource view. Values comply with D3D_RESOURCE_RETURN_TYPE
enum class StorageResourceDataType
{
    unorm = 1,
    snorm,
    sint,
    uint,
    float32,
    unknown,
    float64,
    continued
};

//! Dimension of a typed resource view
enum class ResourceDimension
{
    texture1d,
    texture2d,
    texture3d,
    buffer,
    none
};

//! Shader resource view kinds
enum class TextureResourceType
{
    resource_with_dimension,
    tbuffer,
    structured_buffer,
    raw_buffer
};

//! Unordered access view kinds
enum class StorageBlockResourceType
{
    resource_with_dimension,
    structured_buffer,
    structured_buffer_with_counter,
    raw_buffer,
    append_structured_buffer,
    consume_structured_buffer
};

//! View requirements of a reflected shader resource view
struct TextureShaderInputInfo
{
    bool is_cube = false;
    bool is_ms = false;
    bool is_array = false;
    bool is_buffer = false;
    uint32_t ms_count = 0;
    TextureResourceType resource_type = TextureResourceType::resource_with_dimension;
    ResourceDimension dimension = ResourceDimension::none;
    StorageResourceDataType data_type = StorageResourceDataType::unknown;

    bool operator==(TextureShaderInputInfo const&) const = default;
};

//! View requirements of a reflected unordered access view
struct StorageBlockShaderInputInfo
{
    bool is_ms = false;
    bool is_array = false;
    bool is_buffer = false;
    StorageBlockResourceType resource_type = StorageBlockResourceType::resource_with_dimension;
    ResourceDimension dimension = ResourceDimension::none;
    StorageResourceDataType data_type = StorageResourceDataType::unknown;

    bool operator==(StorageBlockShaderInputInfo const&) const = default;
};

//! View requirements of a reflected shader input. Constant buffers and samplers carry no view requirements
using ViewRequirements = std::variant<std::monostate, TextureShaderInputInfo, StorageBlockShaderInputInfo>;

//! Shader input declared by a single shader stage
struct ReflectedDeclaration
{
    ShaderType stage;
    misc::HashedString name;
    ShaderBindingPoint binding;
    ViewRequirements view;
};

//! Update domains of shader inputs. Each register space belongs to exactly one domain
enum class BindingDomain
{
    pass,    //!< inputs updated per pass or per frame (register spaces 0-9)
    material,    //!< inputs persisting for the lifetime of a material (register spaces 10-19)
    bindless,    //!< unbounded arrays spanning the whole descriptor heap (register spaces 20-29)
    root_constants    //!< constant buffers bound directly to root signature parameters (register space 100)
};

//! Returns binding domain of the given register space or an empty optional if the space is not assigned to any domain
std::optional<BindingDomain> bindingDomainOfRegisterSpace(uint32_t register_space);

using DescriptorTableId = uint32_t;

//! Descriptor table of a compiled binding layout
struct DescriptorTableLayout
{
    BindingDomain domain;
    d3d12::DescriptorHeapType heap_type;
    d3d12::ShaderVisibility visibility;
    d3d12::RootEntryDescriptorTable declaration;    //!< register ranges of the table with explicit offsets from the table start
    uint32_t descriptor_count;    //!< number of descriptors occupied by the table; equals 0 for bindless tables, which span the whole heap
    uint32_t root_slot;
};

//! Constant buffer bound directly to a root signature parameter
struct RootConstantBufferLayout
{
    uint32_t shader_register;
    uint32_t root_slot;
};

//! Location of a named shader input within the descriptor tables of a compiled binding layout
struct BindingPlacement
{
    DescriptorTableId table;
    uint32_t first_descriptor;    //!< offset of the first descriptor of the input from the start of its table
    uint32_t capacity;    //!< number of descriptors available to the input; equals 0 for unbounded arrays
    bool is_unbounded;
    ShaderInputKind kind;
    ViewRequirements view;
};

//! Root signature layout and descriptor placements compiled from reflected shader inputs
struct CompiledBindingLayout
{
    std::vector<RootConstantBufferLayout> root_constant_buffers;
    std::vector<DescriptorTableLayout> descriptor_tables;    //!< indexed by DescriptorTableId
    std::unordered_map<misc::HashedString, BindingPlacement> bindings;
};

//! Either the compiled binding layout or description of the error that prevented compilation
using BindingLayoutCompilationResult = std::variant<CompiledBindingLayout, std::string>;

/*! Compiles binding layout of a shader function from @param declarations reflected from its stages.
 Constant buffers declared in register space c_root_constant_buffer_register_space are bound to root signature
 parameters occupying the first root slots in the order given by @param root_constant_buffer_registers.
 The remaining inputs are packed into one descriptor table per binding domain and descriptor heap type.
 A register occupied in several stages denotes the same input in all of them.
*/
BindingLayoutCompilationResult compileBindingLayout(std::vector<ReflectedDeclaration> const& declarations,
    std::vector<uint32_t> const& root_constant_buffer_registers);

}

#endif
