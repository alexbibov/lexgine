#include "root_signature.h"
#include "d3d12_tools.h"
#include "lexgine/core/exception.h"

#include <algorithm>
#include <functional>

using namespace lexgine::core;
using namespace lexgine::core::misc;
using namespace lexgine::core::dx::d3d12;


RootEntryCBVDescriptor::RootEntryCBVDescriptor(uint32_t shader_register, uint32_t register_space):
    m_shader_register{ shader_register },
    m_register_space{ register_space }
{
}


RootEntryUAVDescriptor::RootEntryUAVDescriptor(uint32_t shader_register, uint32_t register_space) :
    m_shader_register{ shader_register },
    m_register_space{ register_space }
{
}


RootEntrySRVDescriptor::RootEntrySRVDescriptor(uint32_t shader_register, uint32_t register_space) :
    m_shader_register{ shader_register },
    m_register_space{ register_space }
{
}


RootEntryConstants::RootEntryConstants(uint32_t shader_register, uint32_t register_space, uint32_t num_32bit_values):
    m_shader_register{ shader_register },
    m_register_space{ register_space },
    m_num_32bit_values{ num_32bit_values }
{
}


RootEntryDescriptorTable::RootEntryDescriptorTable(std::list<Range> const & ranges):
    m_ranges{ ranges }
{
}

void RootEntryDescriptorTable::addRange(Range const& range)
{
    m_ranges.push_back(range);
}

void RootEntryDescriptorTable::addRange(RangeType type, uint32_t num_descriptors, uint32_t base_register, uint32_t register_space, uint32_t offset_from_start)
{
    m_ranges.emplace_back(type, num_descriptors, base_register, register_space, offset_from_start);
}

RootEntryDescriptorTable::Range::Range(RangeType type, uint32_t num_descriptors, uint32_t base_register, uint32_t register_space, uint32_t offset) :
    type{ type },
    num_descriptors{ num_descriptors },
    base_register{ base_register },
    register_space{ register_space },
    offset{ offset }
{
}



void RootSignature::reset()
{
    m_root_parameters.clear();
    m_descriptor_range_cache.clear();
}

D3DDataBlob RootSignature::compile(RootSignatureFlags const& flags) const
{
    // validate root signature (the slots should follow in order beginning from 0)
    for(uint32_t i = 0U; i < static_cast<uint32_t>(m_root_parameters.size()); ++i)
        if (m_root_parameters.find(i) == m_root_parameters.end())
        {
            std::string err_msg{ "Root signature " + getStringName() + " has undefined slots" };
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, err_msg);
        }

    D3D12_ROOT_SIGNATURE_DESC root_desc;
    root_desc.NumParameters = static_cast<UINT>(m_root_parameters.size());
    root_desc.NumStaticSamplers = static_cast<UINT>(m_static_samplers.size());

    std::vector<D3D12_ROOT_PARAMETER> rs_parameters(root_desc.NumParameters);
    std::vector<D3D12_STATIC_SAMPLER_DESC> rs_static_samplers(root_desc.NumStaticSamplers);

    for (uint32_t i = 0; i < root_desc.NumParameters; ++i) rs_parameters[i] = m_root_parameters.at(i);

    uint32_t sampler_idx = 0;
    for (auto p = m_static_samplers.begin(); p != m_static_samplers.end(); ++p, ++sampler_idx) rs_static_samplers[sampler_idx] = *p;


    root_desc.pParameters = rs_parameters.data();
    root_desc.pStaticSamplers = rs_static_samplers.data();
    root_desc.Flags = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(flags.getValue());


    ID3DBlob* serialized_rs = nullptr, *error = nullptr;
    if (D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized_rs, &error) != S_OK)
    {
        std::string serialization_error{ static_cast<char*>(error->GetBufferPointer()), error->GetBufferSize() };
        std::string err_msg = "Unable to serialize root signature: " + serialization_error;

        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, err_msg);
    }

    D3DDataBlob rv{serialized_rs};
    serialized_rs->Release();
    if(error) error->Release();

    return rv;
}

RootSignature& RootSignature::addParameter(uint32_t slot, RootEntryCBVDescriptor const& root_entry_cbv_descriptor_declaration, ShaderVisibility shader_visibility)
{
    D3D12_ROOT_PARAMETER param;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor = D3D12_ROOT_DESCRIPTOR{ root_entry_cbv_descriptor_declaration.m_shader_register, root_entry_cbv_descriptor_declaration.m_register_space };
    param.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(shader_visibility);

    m_root_parameters.emplace(slot, param);

    return *this;
}

RootSignature& RootSignature::addParameter(uint32_t slot, RootEntryUAVDescriptor const& root_entry_uav_descriptor_declaration, ShaderVisibility shader_visibility)
{
    D3D12_ROOT_PARAMETER param;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    param.Descriptor = D3D12_ROOT_DESCRIPTOR{ root_entry_uav_descriptor_declaration.m_shader_register, root_entry_uav_descriptor_declaration.m_register_space };
    param.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(shader_visibility);

    m_root_parameters.emplace(slot, param);

    return *this;
}

RootSignature& RootSignature::addParameter(uint32_t slot, RootEntrySRVDescriptor const& root_entry_srv_descriptor_declaration, ShaderVisibility shader_visibility)
{
    D3D12_ROOT_PARAMETER param;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    param.Descriptor = D3D12_ROOT_DESCRIPTOR{ root_entry_srv_descriptor_declaration.m_shader_register, root_entry_srv_descriptor_declaration.m_register_space };
    param.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(shader_visibility);

    m_root_parameters.emplace(slot, param);

    return *this;
}

RootSignature& RootSignature::addParameter(uint32_t slot, RootEntryConstants const& root_entry_constants_declaration, ShaderVisibility shader_visibility)
{
    D3D12_ROOT_PARAMETER param;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param.Constants = D3D12_ROOT_CONSTANTS{ root_entry_constants_declaration.m_shader_register,
        root_entry_constants_declaration.m_register_space,
        root_entry_constants_declaration.m_num_32bit_values };
    param.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(shader_visibility);

    m_root_parameters.emplace(slot, param);

    return *this;

}

RootSignature& RootSignature::addParameter(uint32_t slot, RootEntryDescriptorTable const& root_entry_descriptor_table_declaration, ShaderVisibility shader_visibility)
{
    D3D12_ROOT_PARAMETER param;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;


    size_t num_ranges = root_entry_descriptor_table_declaration.m_ranges.size();
    param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(num_ranges);

    size_t current_offset_in_cache = m_descriptor_range_cache.size();
    m_descriptor_range_cache.resize(current_offset_in_cache + num_ranges);
    auto range_iterator = root_entry_descriptor_table_declaration.m_ranges.begin();
    for (size_t i = current_offset_in_cache; i < current_offset_in_cache + num_ranges; ++i, ++range_iterator)
    {
        m_descriptor_range_cache[i].RangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(range_iterator->type);
        m_descriptor_range_cache[i].NumDescriptors = range_iterator->num_descriptors;
        m_descriptor_range_cache[i].BaseShaderRegister = range_iterator->base_register;
        m_descriptor_range_cache[i].RegisterSpace = range_iterator->register_space;
        m_descriptor_range_cache[i].OffsetInDescriptorsFromTableStart = range_iterator->offset;
    }
    param.DescriptorTable.pDescriptorRanges = m_descriptor_range_cache.data() + current_offset_in_cache;


    param.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(shader_visibility);

    m_root_parameters.emplace(slot, param);

    return *this;
}

RootSignature& RootSignature::addParameter(uint32_t slot, RootStaticSampler const& root_static_sampler_declaration, ShaderVisibility shader_visibility)
{
    D3D12_STATIC_SAMPLER_DESC desc;
    desc.Filter = d3d12Convert(root_static_sampler_declaration.m_filter_pack.MinFilter(),
        root_static_sampler_declaration.m_filter_pack.MagFilter(),
        root_static_sampler_declaration.m_filter_pack.isComparison());
    auto uv_wrap_modes = root_static_sampler_declaration.m_filter_pack.getWrapModeUV();
    desc.AddressU = d3d12Convert(uv_wrap_modes.first);
    desc.AddressV = d3d12Convert(uv_wrap_modes.second);
    desc.AddressW = d3d12Convert(root_static_sampler_declaration.m_filter_pack.getWrapModeW());
    desc.MipLODBias = root_static_sampler_declaration.m_filter_pack.getLODBias();
    desc.MaxAnisotropy = root_static_sampler_declaration.m_filter_pack.getAnisotropyLevel();
    desc.ComparisonFunc = d3d12Convert(root_static_sampler_declaration.m_filter_pack.getComparisonFunction());
    desc.BorderColor = d3d12Convert(root_static_sampler_declaration.m_filter_pack.getBorderColor());
    auto min_max_log = root_static_sampler_declaration.m_filter_pack.getMinMaxLOD();
    desc.MinLOD = min_max_log.first;
    desc.MaxLOD = min_max_log.second;
    desc.ShaderRegister = root_static_sampler_declaration.m_shader_register;
    desc.RegisterSpace = root_static_sampler_declaration.m_register_space;
    desc.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(shader_visibility);

    m_static_samplers.push_back(desc);

    return *this;
}

RootStaticSampler::RootStaticSampler(uint32_t shader_register, uint32_t register_space, FilterPack const& filter_pack):
    m_shader_register{ shader_register },
    m_register_space{ register_space },
    m_filter_pack{ filter_pack }
{
}
