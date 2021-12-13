#ifndef LEXGINE_CORE_DX_D3D12_ROOT_SIGNATURE_H
#define LEXGINE_CORE_DX_D3D12_ROOT_SIGNATURE_H

#include <map>
#include <list>
#include <vector>

#include <d3d12.h>
#include <wrl.h>

#include "engine/core/data_blob.h"
#include "engine/core/entity.h"
#include "engine/core/class_names.h"
#include "engine/core/filter.h"
#include "engine/core/misc/flags.h"


using namespace Microsoft::WRL;

namespace lexgine::core::dx::d3d12 {

//! Root constant buffer view descriptor
class RootEntryCBVDescriptor final
{
    friend class RootSignature;

public:
    RootEntryCBVDescriptor(uint32_t shader_register, uint32_t register_space);

private:
    uint32_t m_shader_register;
    uint32_t m_register_space;
};

//! Root unordered access view descriptor
class RootEntryUAVDescriptor final
{
    friend class RootSignature;

public:
    RootEntryUAVDescriptor(uint32_t shader_register, uint32_t register_space);

private:
    uint32_t m_shader_register;
    uint32_t m_register_space;
};

//! Root shader resource view descriptor
class RootEntrySRVDescriptor final
{
    friend class RootSignature;

public:
    RootEntrySRVDescriptor(uint32_t shader_register, uint32_t register_space);

private:
    uint32_t m_shader_register;
    uint32_t m_register_space;
};

//! Root constants
class RootEntryConstants final
{
    friend class RootSignature;

public:
    RootEntryConstants(uint32_t shader_register, uint32_t register_space, uint32_t num_32bit_values);

private:
    uint32_t m_shader_register;    //!< base "b" shader register
    uint32_t m_register_space;    //!< register space used in shaders
    uint32_t m_num_32bit_values;    //!< number of 32-bit values (viewed as a single constant buffer on the shader side) that will occupy single root signature slot
};

//! Descriptor table entry
class RootEntryDescriptorTable final
{
    friend class RootSignature;

public:
    enum class RangeType : uint8_t
    {
        srv, uav, cbv, sampler      // note: the order is important for compliance with D3D12 API
    };

    struct Range
    {
        RangeType type;    //!< type of virtual register range contained in descriptor table
        uint32_t num_descriptors;    //!< number of descriptors contained in descriptor table
        uint32_t base_register;    //!< first register from which to begin binding the descriptors
        uint32_t register_space;    //!< virtual register space in which to bind the descriptors
        uint32_t offset;    //!< offset from the physical table start where this range should be bound

        Range(RangeType type, uint32_t num_descriptors, uint32_t base_register, uint32_t register_space, uint32_t offset);
    };


    RootEntryDescriptorTable() = default;

    RootEntryDescriptorTable(std::list<Range> const& ranges);    //! initializes descriptor table using provided register ranges

    //! adds new register range to descriptor table
    void addRange(Range const& range);

    //! adds new register range to descriptor table
    void addRange(RangeType type, uint32_t num_descriptors, uint32_t base_register, uint32_t register_space, uint32_t offset_from_start);


private:
    std::list<Range> m_ranges;    //!< register ranges contained in the table
};


//! Root signature static sampler descriptor
class RootStaticSampler final
{
    friend class RootSignature;

public:
    RootStaticSampler(uint32_t shader_register, uint32_t register_space, FilterPack const& filter_pack);

private:
    uint32_t m_shader_register;    //! "s" virtual register occupied by the static sampler
    uint32_t m_register_space;    //! register space, in which the static sampler occupies its virtual shader register
    FilterPack m_filter_pack;    //! filter parameters used by the sampler
};


//! Enumerates available restrictions on shader visibility of root parameters. Complies with D3D12 constant definitions
enum class ShaderVisibility : int
{
    all,    //!< visible to all shading stages
    vertex,    //!< only vertex shading stage has access to the root parameter
    hull,    //!< only hull (tessellation control in Khronos terms) shading stage has access to the root parameter
    domain,    //!< only domain (tessellation evaluation in Khronos terms) shading stage has access to the root parameter
    geometry,    //!< only geometry shading stage has access to the root parameter
    pixel    //!< only pixel (fragment in Khronos terms) shading stage has access to the root parameter
};


BEGIN_FLAGS_DECLARATION(RootSignatureFlags)
FLAG(none, D3D12_ROOT_SIGNATURE_FLAG_NONE)
FLAG(allow_input_assembler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
FLAG(deny_vertex_shader, D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS)
FLAG(deny_hull_shader, D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS)
FLAG(deny_domain_shader, D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS)
FLAG(deny_geometry_shader, D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS)
FLAG(deny_pixel_shader, D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS)
FLAG(allow_stream_output, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT)
END_FLAGS_DECLARATION(RootSignatureFlags);


//! Wrapper that simplifies procedural creation of root signatures
class RootSignature final : public NamedEntity<class_names::D3D12_RootSignature>
{
public:
    RootSignature() = default;
    RootSignature(RootSignature const&) = delete;
    RootSignature(RootSignature&&) = default;
    RootSignature& operator=(RootSignature const&) = delete;
    RootSignature& operator=(RootSignature&&) = delete;


    void reset();     //! erases current root signature declaration. After calling this function the root signature is empty as if it was just initialized

    //! compiles root signature and returns it packed into a data blob
    D3DDataBlob compile(RootSignatureFlags const& flags = RootSignatureFlags::base_values::allow_input_assembler) const;

    //! adds new root CBV descriptor into the given slot of the root signature
    RootSignature& addParameter(uint32_t slot, RootEntryCBVDescriptor const& root_entry_cbv_descriptor_declaration, ShaderVisibility shader_visibility = ShaderVisibility::all);

    //! adds new root UAV descriptor into the given slot of the root signature
    RootSignature& addParameter(uint32_t slot, RootEntryUAVDescriptor const& root_descriptor_entry_declaration, ShaderVisibility shader_visibility = ShaderVisibility::all);

    //! adds new root SRV descriptor into the given slot of the root signature
    RootSignature& addParameter(uint32_t slot, RootEntrySRVDescriptor const& root_descriptor_entry_declaration, ShaderVisibility shader_visibility = ShaderVisibility::all);

    //! adds new root constants entry into the given slot of the root signature
    RootSignature& addParameter(uint32_t slot, RootEntryConstants const& root_entry_constants_declaration, ShaderVisibility shader_visibility = ShaderVisibility::all);

    //! adds new descriptor table entry
    RootSignature& addParameter(uint32_t slot, RootEntryDescriptorTable const& root_entry_descriptor_table_declaration, ShaderVisibility shader_visibility = ShaderVisibility::all);

    //! adds new static sampler into the root signature
    RootSignature& addStaticSampler(RootStaticSampler const& root_static_sampler_declaration, ShaderVisibility shader_visibility = ShaderVisibility::all);

private:
    std::unordered_map<uint32_t, D3D12_ROOT_PARAMETER> m_root_parameters;    //!< root signature parameters packed into a map with the key defining slot in the root signature
    std::list<std::vector<D3D12_DESCRIPTOR_RANGE>> m_descriptor_range_cache;    //!< stores descriptor ranges from all descriptor tables
    std::list<D3D12_STATIC_SAMPLER_DESC> m_static_samplers;    //!< list of static samplers attached to the root signature
};

}

#endif
