
#include "engine/core/exception.h"
#include "engine/core/globals.h"
#include "engine/core/data_blob.h"
#include "engine/core/misc/log.h"
#include "engine/core/misc/strict_weak_ordering.h"
#include "engine/core/dx/d3d12/task_caches/root_signature_compilation_task_cache.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"

#include "shader_stage.h"

namespace lexgine::core::dx::dxcompilation 
{

namespace
{

d3d12::ConstantBufferReflection::ReflectionEntryBaseType getReflectionEntryType(D3D12_SHADER_TYPE_DESC const& type_desc)
{
    size_t type_offset_in_reflection_table{ 0 };
    switch (type_desc.Type) {
    case D3D_SVT_FLOAT:
        type_offset_in_reflection_table = 0;
        break;
    case D3D_SVT_INT:
        type_offset_in_reflection_table = 1;
        break;
    case D3D_SVT_UINT:
        type_offset_in_reflection_table = 2;
        break;
    case D3D_SVT_BOOL:
        type_offset_in_reflection_table = 3;
        break;
    default:
        LEXGINE_ASSUME;
    }

    switch (type_desc.Class) {
    case D3D_SVC_SCALAR:
        return static_cast<d3d12::ConstantBufferReflection::ReflectionEntryBaseType>(type_offset_in_reflection_table);
       
    case D3D_SVC_VECTOR:
        return static_cast<d3d12::ConstantBufferReflection::ReflectionEntryBaseType>(4 + (type_desc.Columns - 2) * 16 + type_offset_in_reflection_table);
        
    case D3D_SVC_MATRIX_ROWS:
    case D3D_SVC_MATRIX_COLUMNS:
        return static_cast<d3d12::ConstantBufferReflection::ReflectionEntryBaseType>(8 + (type_desc.Rows - 2) * 16 + (type_desc.Columns - 2) * 4 + type_offset_in_reflection_table);
        
    default:
        LEXGINE_ASSUME;

    }
    
    return d3d12::ConstantBufferReflection::ReflectionEntryBaseType::unknown;
}

void collectStructReflection(ID3D12ShaderReflectionType* p_type_reflection, D3D12_SHADER_TYPE_DESC const& type_desc, d3d12::ConstantBufferReflection& cb_reflection)
{
    assert(type_desc.Class == D3D_SVC_STRUCT);

    for (unsigned i = 0; i < type_desc.Members; ++i)
    {
        ID3D12ShaderReflectionType* p_member_type_reflection = p_type_reflection->GetMemberTypeByIndex(static_cast<UINT>(i));
        D3D12_SHADER_TYPE_DESC member_type_desc{};
        p_member_type_reflection->GetDesc(&member_type_desc);

        if (member_type_desc.Class == D3D_SVC_STRUCT)
        {
            collectStructReflection(p_member_type_reflection, member_type_desc, cb_reflection);
        }
        else
        {
            d3d12::ConstantBufferReflection::ReflectionEntryDesc entry_desc{};
            entry_desc.base_type = getReflectionEntryType(member_type_desc);
            entry_desc.element_count = (std::max)(static_cast<size_t>(1), static_cast<size_t>(member_type_desc.Elements));
            cb_reflection.addElement(p_type_reflection->GetMemberTypeName(static_cast<UINT>(i)), entry_desc);
        }
    }
}

}

unsigned int ShaderStage::getInstructionCount()
{
    return static_cast<unsigned int>(m_shader_desc.InstructionCount);
}

bool ShaderStage::bindConstantBuffer(const misc::HashedString& name, const d3d12::Resource& buffer, uint32_t offset, uint32_t size_in_bytes)
{
    if (!m_shader_resource_names_pool.contains(name))
    {
        return false;
    }
    assert(m_shader_resource_names_pool[name].kind == ShaderFunction::ShaderInputKind::cbv);
    auto descriptor_pointer = ShaderFunctionAttorney<ShaderStage>::getStaticCbvSrvUavDescriptorAllocator(*m_owning_shader_function_ptr).getOrCreateDescriptor(d3d12::CBVDescriptor{buffer, offset, size_in_bytes});
    ShaderFunctionAttorney<ShaderStage>::bindResourceToShaderFunctionInput(*m_owning_shader_function_ptr, getShaderType(), m_shader_resource_names_pool[name], descriptor_pointer);
    return true;
}

bool ShaderStage::bindTextureBuffer(const misc::HashedString& name, const d3d12::Resource& buffer_texture, uint64_t first_buffer_element, uint32_t buffer_element_stride)
{
    if (!m_shader_resource_names_pool.contains(name)) {
        return false;
    }
    assert(m_shader_resource_names_pool[name].kind == ShaderFunction::ShaderInputKind::srv);

    TextureShaderInputInfo& texture_info = m_texture_shader_inputs[m_shader_resource_names_pool[name]];
    assert(texture_info.resource_type == TextureResourceType::tbuffer
        || texture_info.resource_type == TextureResourceType::structured_buffer
        || texture_info.resource_type == TextureResourceType::raw_buffer);

    d3d12::ResourceDescriptor const& resource_desc = buffer_texture.descriptor();
    assert(resource_desc.dimension == d3d12::ResourceDimension::buffer);

    if (texture_info.resource_type == TextureResourceType::tbuffer) {
        buffer_element_stride = getDataTypeSize(texture_info.data_type);
    }
    else if (texture_info.resource_type == TextureResourceType::raw_buffer) 
    {
        buffer_element_stride = 1;
    }

    d3d12::SRVBufferInfo info {
        .first_element = first_buffer_element,
        .num_elements = static_cast<uint32_t>((resource_desc.width - first_buffer_element * buffer_element_stride) / buffer_element_stride),
        .structure_byte_stride = buffer_element_stride,
        .flags = texture_info.resource_type == TextureResourceType::raw_buffer ? d3d12::SRVBufferInfoFlags::raw : d3d12::SRVBufferInfoFlags::none
    };
    d3d12::SRVDescriptor srv_descriptor{ buffer_texture, info };
    if (texture_info.resource_type == TextureResourceType::raw_buffer)
    {
        srv_descriptor.overrideFormat(DXGI_FORMAT_R32_TYPELESS);
    }

    auto descriptor_pointer = ShaderFunctionAttorney<ShaderStage>::getStaticCbvSrvUavDescriptorAllocator(*m_owning_shader_function_ptr).getOrCreateDescriptor(srv_descriptor);
    ShaderFunctionAttorney<ShaderStage>::bindResourceToShaderFunctionInput(*m_owning_shader_function_ptr, getShaderType(), m_shader_resource_names_pool[name], descriptor_pointer);
    return true;
}

bool ShaderStage::bindTexture(const misc::HashedString& name, const d3d12::Resource& texture)
{
    if (!m_shader_resource_names_pool.contains(name)) 
    {
        return false;
    }
    assert(m_shader_resource_names_pool[name].kind == ShaderFunction::ShaderInputKind::srv);

    ShaderFunction::ShaderBindingPoint const& binding_point_desc = m_shader_resource_names_pool[name];
    TextureShaderInputInfo& texture_info = m_texture_shader_inputs[binding_point_desc];

    d3d12::StaticDescriptorAllocationManager::UPointer descriptor_pointer{};
    if (binding_point_desc.register_count > 1)
    {
        // bound resource is an array
        d3d12::SRVTextureArrayInfo info{};
        info.num_array_elements = binding_point_desc.register_count;
        descriptor_pointer = ShaderFunctionAttorney<ShaderStage>::getStaticCbvSrvUavDescriptorAllocator(*m_owning_shader_function_ptr).getOrCreateDescriptor(d3d12::SRVDescriptor{texture, info});
    }
    else
    {
        d3d12::SRVTextureInfo info{};
        descriptor_pointer = ShaderFunctionAttorney<ShaderStage>::getStaticCbvSrvUavDescriptorAllocator(*m_owning_shader_function_ptr).getOrCreateDescriptor(d3d12::SRVDescriptor{ texture, info });
        
    }

    ShaderFunctionAttorney<ShaderStage>::bindResourceToShaderFunctionInput(*m_owning_shader_function_ptr, getShaderType(), binding_point_desc, descriptor_pointer);
    return true;
}

bool ShaderStage::bindStorageBlock(const misc::HashedString& name, const d3d12::Resource& storage_block, uint64_t first_buffer_element, uint32_t buffer_element_stride)
{
    if (!m_shader_resource_names_pool.contains(name))
    {
        return false;
    }
    assert(m_shader_resource_names_pool[name].kind == ShaderFunction::ShaderInputKind::uav);

    ShaderFunction::ShaderBindingPoint const& binding_point_desc = m_shader_resource_names_pool[name];
    StorageBlockShaderInputInfo& storage_block_info = m_storage_block_inputs[binding_point_desc];
    
    d3d12::Resource* pUAVCounterResource = nullptr;
    uint64_t counter_offset_in_bytes = 0;

    d3d12::StaticDescriptorAllocationManager::UPointer descriptor_pointer{};
    switch (storage_block_info.resource_type)
    {
    case StorageBlockResourceType::typed:
    {
        if (binding_point_desc.register_count > 1)
        {
            // shader input resource is an array
            d3d12::UAVTextureArrayInfo info{};
            info.num_array_elements = binding_point_desc.register_count;
            descriptor_pointer = ShaderFunctionAttorney<ShaderStage>::getStaticCbvSrvUavDescriptorAllocator(*m_owning_shader_function_ptr).getOrCreateDescriptor(d3d12::UAVDescriptor{storage_block, info});
        }
        else
        {
            d3d12::UAVTextureInfo info{};
            descriptor_pointer = ShaderFunctionAttorney<ShaderStage>::getStaticCbvSrvUavDescriptorAllocator(*m_owning_shader_function_ptr).getOrCreateDescriptor(d3d12::UAVDescriptor{ storage_block, info });
        }
        break;
    }

    case StorageBlockResourceType::structured_buffer_with_counter:
    case StorageBlockResourceType::append_structured_buffer:
    case StorageBlockResourceType::consume_structured_buffer:
        pUAVCounterResource = ShaderFunctionAttorney<ShaderStage>::getUavAtomicCountersResource(*m_owning_shader_function_ptr);
        counter_offset_in_bytes = ShaderFunctionAttorney<ShaderStage>::stepUavCounterOffset(*m_owning_shader_function_ptr);
        [[fallthrough]];
    case StorageBlockResourceType::structured_buffer:
    case StorageBlockResourceType::raw_buffer:
    {
        d3d12::ResourceDescriptor const& resource_desc = storage_block.descriptor();
        d3d12::UAVBufferInfo info{
            .first_element = 0,
            .num_elements = static_cast<uint32_t>((resource_desc.width - first_buffer_element * buffer_element_stride) / buffer_element_stride),
            .structure_byte_stride = buffer_element_stride,
            .counter_offset_in_bytes = counter_offset_in_bytes,
            .flags = storage_block_info.resource_type == StorageBlockResourceType::raw_buffer ? d3d12::UnorderedAccessViewBufferInfoFlags::raw : d3d12::UnorderedAccessViewBufferInfoFlags::none
        };
        d3d12::UAVDescriptor uav_descriptor{ storage_block, info, pUAVCounterResource };
        if (storage_block_info.resource_type == StorageBlockResourceType::raw_buffer)
        {
            uav_descriptor.overrideFormat(DXGI_FORMAT_R32_TYPELESS);
        }
        descriptor_pointer = ShaderFunctionAttorney<ShaderStage>::getStaticCbvSrvUavDescriptorAllocator(*m_owning_shader_function_ptr).getOrCreateDescriptor(uav_descriptor);
        break;
    }
    
    default:
        LEXGINE_ASSUME;
    }

    ShaderFunctionAttorney<ShaderStage>::bindResourceToShaderFunctionInput(*m_owning_shader_function_ptr, getShaderType(), binding_point_desc, descriptor_pointer);
    return true;
}

bool ShaderStage::bindSampler(misc::HashedString const& name, FilterPack const& filter, math::Vector4f const& border_color)
{
    if (!m_shader_resource_names_pool.contains(name))
    {
        return false;
    }
    assert(m_shader_resource_names_pool[name].kind == ShaderFunction::ShaderInputKind::sampler);
    auto descriptor_pointer = ShaderFunctionAttorney<ShaderStage>::getStaticSamplerDescriptorAllocator(*m_owning_shader_function_ptr).getOrCreateDescriptor(d3d12::SamplerDescriptor{ filter, border_color });
    ShaderFunctionAttorney<ShaderStage>::bindResourceToShaderFunctionInput(*m_owning_shader_function_ptr, getShaderType(), m_shader_resource_names_pool[name], descriptor_pointer);
    return true;
}

d3d12::ConstantBufferReflection const ShaderStage::buildConstantBufferReflection(misc::HashedString const& constant_buffer_name) const
{
    d3d12::ConstantBufferReflection rv{};

    ShaderFunction::ShaderBindingPoint binding_point_desc = m_shader_resource_names_pool.at(constant_buffer_name);
    assert(binding_point_desc.kind == ShaderFunction::ShaderInputKind::cbv);
    

    ID3D12ShaderReflectionConstantBuffer* p_constant_buffer_reflection = m_shader_reflection->GetConstantBufferByName(constant_buffer_name.string());
    assert(p_constant_buffer_reflection);

    D3D12_SHADER_BUFFER_DESC constant_buffer_desc {};
    p_constant_buffer_reflection->GetDesc(&constant_buffer_desc);
    for (unsigned i = 0; i < static_cast<unsigned>(constant_buffer_desc.Variables); ++i) {
        ID3D12ShaderReflectionVariable* p_variable_reflextion = p_constant_buffer_reflection->GetVariableByIndex(i);

        D3D12_SHADER_VARIABLE_DESC variable_desc {};
        p_variable_reflextion->GetDesc(&variable_desc);

        D3D12_SHADER_TYPE_DESC variable_type_desc {};
        ID3D12ShaderReflectionType* p_variable_type_reflection = p_variable_reflextion->GetType();
        p_variable_type_reflection->GetDesc(&variable_type_desc);

        if (variable_type_desc.Class == D3D_SVC_STRUCT) {
            collectStructReflection(p_variable_type_reflection, variable_type_desc, rv);
        } else {
            d3d12::ConstantBufferReflection::ReflectionEntryDesc entry_desc {};
            entry_desc.base_type = getReflectionEntryType(variable_type_desc);
            entry_desc.element_count = (std::max)(static_cast<size_t>(1), static_cast<size_t>(variable_type_desc.Elements));
            rv.addElement(variable_desc.Name, entry_desc);
        }
    }

    return rv;
}

ShaderType ShaderStage::getShaderType() const
{
    switch ((m_shader_desc.Version & 0xffff0000) >> 16)
    {
    case D3D12_SHVER_PIXEL_SHADER:
        return ShaderType::pixel;

    case D3D12_SHVER_VERTEX_SHADER:
        return ShaderType::vertex;

    case D3D12_SHVER_GEOMETRY_SHADER:
        return ShaderType::geometry;

    case D3D12_SHVER_HULL_SHADER:
        return ShaderType::hull;

    case D3D12_SHVER_DOMAIN_SHADER:
        return ShaderType::domain;

    case D3D12_SHVER_COMPUTE_SHADER:
        return ShaderType::compute;

    default:
        return ShaderType::unknown;
    }

    return ShaderType::unknown;
}

ShaderArgumentInfo const& ShaderStage::getShaderArgumentInfo(ShaderArgumentKind kind, ShaderArgumentInfoKey const& key) const
{
    std::unordered_map<ShaderArgumentInfoKey, ShaderArgumentInfo> const* p_target_map{};
    switch (kind)
    {
    case ShaderArgumentKind::input:
        p_target_map = &m_shader_input_arguments;
        break;

    case ShaderArgumentKind::output:
        p_target_map = &m_shader_output_arguments;
        break;

    default:
        LEXGINE_ASSUME;
    }

    assert(p_target_map);
    return p_target_map->at(key);
}

std::unordered_map<ShaderArgumentInfoKey, ShaderArgumentInfo> const& ShaderStage::getShaderArguments(ShaderArgumentKind kind) const
{
    switch (kind)
    {
    case ShaderArgumentKind::input:
        return m_shader_input_arguments;
    case ShaderArgumentKind::output:
        return m_shader_output_arguments;
    default:
        LEXGINE_ASSUME;
    }

    return m_shader_input_arguments;
}

ShaderStage::ShaderStage(Globals const& globals, d3d12::tasks::HLSLCompilationTask const* p_shader_compilation_task, ShaderFunction* p_owning_shader_function)
    : m_globals{ globals }
    , m_shader_name { p_shader_compilation_task->getStringName() }
    , m_owning_shader_function_ptr{ p_owning_shader_function }
{
    if (!p_shader_compilation_task->isCompleted()) {
        misc::Log::retrieve()->out("Unable to create reflection for shader: shader compilation task is not completed", misc::LogMessageType::exclamation);
        return;
    }

    LEXGINE_LOG_ERROR_IF_FAILED(this, DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_dxc_utils.GetAddressOf())), S_OK);
    if (getErrorState()) {
        misc::Log::retrieve()->out("Unable to create reflection for shader '" + p_shader_compilation_task->getStringName() + "': DxcUtils creation failed", misc::LogMessageType::exclamation);
        return;
    }

    DxcBuffer hlsl_source_dxc_buffer { .Ptr = p_shader_compilation_task->getTaskData().data(), .Size = p_shader_compilation_task->getTaskData().size(), .Encoding = 0 };
    LEXGINE_LOG_ERROR_IF_FAILED(this, m_dxc_utils->CreateReflection(&hlsl_source_dxc_buffer, IID_PPV_ARGS(m_shader_reflection.GetAddressOf())), S_OK);
    if (getErrorState()) {
        misc::Log::retrieve()->out("Unable to create reflection for shader '" + p_shader_compilation_task->getStringName() + "': shader reflection creation failed", misc::LogMessageType::exclamation);
        return;
    }

    LEXGINE_LOG_ERROR_IF_FAILED(this, m_shader_reflection->GetDesc(&m_shader_desc), S_OK);
    if (getErrorState()) {
        misc::Log::retrieve()->out("Unable to create reflection for shader '" + p_shader_compilation_task->getStringName() + "': shader reflection description retrieval failed", misc::LogMessageType::exclamation);
        return;
    }

    collectShaderArguments(ShaderArgumentKind::input);
    collectShaderArguments(ShaderArgumentKind::output);
    collectShaderBindings();
}

uint32_t ShaderStage::getDataTypeSize(TextureResourceDataType data_type)
{
    switch (data_type)
    {
    case ShaderStage::TextureResourceDataType::unorm:
    case ShaderStage::TextureResourceDataType::snorm:
    case ShaderStage::TextureResourceDataType::sint:
    case ShaderStage::TextureResourceDataType::uint:
    case ShaderStage::TextureResourceDataType::float32:
        return 4;
   
    case ShaderStage::TextureResourceDataType::float64:
    case ShaderStage::TextureResourceDataType::continued:
        return 8;

    case ShaderStage::TextureResourceDataType::unknown:
        return 0;
    default:
        LEXGINE_ASSUME;
    }
    return 0;
}

void ShaderStage::collectShaderBindings()
{
    for (UINT i = 0; i < m_shader_desc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC desc{};
        m_shader_reflection->GetResourceBindingDesc(i, &desc);

        misc::HashedString hashed_name{ desc.Name };
        assert(!m_shader_resource_names_pool.contains(hashed_name));

        ShaderFunction::ShaderBindingPoint binding_point{};
        binding_point.first_register = static_cast<size_t>(desc.BindPoint);
        binding_point.register_count = static_cast<size_t>(desc.BindCount);
        binding_point.register_space = static_cast<size_t>(desc.Space);

        switch (desc.Type)
        {
        case D3D_SIT_CBUFFER:
            binding_point.kind = ShaderFunction::ShaderInputKind::cbv;
            break;

        case D3D_SIT_SAMPLER:
            binding_point.kind = ShaderFunction::ShaderInputKind::sampler;
            break;

        case D3D_SIT_TEXTURE:
        case D3D_SIT_TBUFFER:
        case D3D_SIT_STRUCTURED:
        case D3D_SIT_BYTEADDRESS:
        {
            binding_point.kind = ShaderFunction::ShaderInputKind::srv;
            TextureShaderInputInfo extra_info{};
            extra_info.ms_count = static_cast<uint32_t>(desc.NumSamples);
            extra_info.data_type = static_cast<TextureResourceDataType>(desc.ReturnType);
            m_texture_shader_inputs[binding_point] = extra_info;
            break;
        }

        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        case D3D_SIT_UAV_RWBYTEADDRESS:
        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        {
            binding_point.kind = ShaderFunction::ShaderInputKind::uav;
            break;
        }

        default:
            LEXGINE_ASSUME;
        }
        m_shader_resource_names_pool.insert(std::make_pair(hashed_name, binding_point));

        switch (binding_point.kind)
        {
        case ShaderFunction::ShaderInputKind::cbv:
        case ShaderFunction::ShaderInputKind::sampler:
            break;

        case ShaderFunction::ShaderInputKind::srv:
        {
            switch (desc.Type)
            {
            case D3D_SIT_TEXTURE:
            {
                switch (desc.Dimension)
                {
                case D3D_SRV_DIMENSION_TEXTURE1D:
                case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
                    m_texture_shader_inputs[binding_point].resource_type = TextureResourceType::texture1d;
                    break;

                case D3D_SRV_DIMENSION_TEXTURE2DMS:
                case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
                    m_texture_shader_inputs[binding_point].is_ms = true;

                case D3D_SRV_DIMENSION_TEXTURE2D:
                case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
                    m_texture_shader_inputs[binding_point].resource_type = TextureResourceType::texture2d;
                    break;
               
                case D3D_SRV_DIMENSION_TEXTURE3D:
                    m_texture_shader_inputs[binding_point].resource_type = TextureResourceType::texture3d;
                    break;

                case D3D_SRV_DIMENSION_TEXTURECUBE:
                case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
                    m_texture_shader_inputs[binding_point].is_cube = true;
                    m_texture_shader_inputs[binding_point].resource_type = TextureResourceType::texture2d;
                    break;

                default:
                    LEXGINE_ASSUME;
                }
                break;
            }

            case D3D_SIT_TBUFFER:
                m_texture_shader_inputs[binding_point].resource_type = TextureResourceType::tbuffer;
                break;

            case D3D_SIT_STRUCTURED:
                m_texture_shader_inputs[binding_point].resource_type = TextureResourceType::structured_buffer;
                break;

            case D3D_SIT_BYTEADDRESS:
                m_texture_shader_inputs[binding_point].resource_type = TextureResourceType::raw_buffer;
                break;

            default:
                LEXGINE_ASSUME;
            }
            break;
        }


        case ShaderFunction::ShaderInputKind::uav:
        {
            switch (desc.Type)
            {
            case D3D_SIT_UAV_RWTYPED:
                m_storage_block_inputs[binding_point].resource_type = StorageBlockResourceType::typed;
                break;

            case D3D_SIT_UAV_RWSTRUCTURED:
                m_storage_block_inputs[binding_point].resource_type = StorageBlockResourceType::structured_buffer;
                break;

            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                m_storage_block_inputs[binding_point].resource_type = StorageBlockResourceType::structured_buffer_with_counter;
                break;

            case D3D_SIT_UAV_RWBYTEADDRESS:
                m_storage_block_inputs[binding_point].resource_type = StorageBlockResourceType::raw_buffer;
                break;

            case D3D_SIT_UAV_APPEND_STRUCTURED:
                m_storage_block_inputs[binding_point].resource_type = StorageBlockResourceType::append_structured_buffer;
                break;

            case D3D_SIT_UAV_CONSUME_STRUCTURED:
                m_storage_block_inputs[binding_point].resource_type = StorageBlockResourceType::consume_structured_buffer;
                break;

            default:
                LEXGINE_ASSUME;
            }
            break;
        }


        default:
            LEXGINE_ASSUME;
        }  
    }
}

void ShaderStage::collectShaderArguments(ShaderArgumentKind kind)
{
    std::unordered_map<ShaderArgumentInfoKey, ShaderArgumentInfo>* p_target_map{};
    int argument_count{};
    HRESULT (ID3D12ShaderReflection::* p_get_parameter_desc)(UINT, D3D12_SIGNATURE_PARAMETER_DESC*) noexcept{};
    switch (kind)
    {
    case ShaderArgumentKind::input:
        p_target_map = &m_shader_input_arguments;
        argument_count = static_cast<int>(m_shader_desc.InputParameters);
        p_get_parameter_desc = &ID3D12ShaderReflection::GetInputParameterDesc;
        break;

    case ShaderArgumentKind::output:
        p_target_map = &m_shader_output_arguments;
        argument_count = static_cast<int>(m_shader_desc.OutputParameters);
        p_get_parameter_desc = &ID3D12ShaderReflection::GetOutputParameterDesc;
        break;

    default:
        LEXGINE_ASSUME;
    }

    for (int i = 0; i < argument_count; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC desc{};
        LEXGINE_LOG_ERROR_IF_FAILED(this, ((m_shader_reflection.Get())->*p_get_parameter_desc)(i, &desc), S_OK);
        if (getErrorState())
        {
            misc::Log::retrieve()->out("Unable to retrieve shader input parameter description for input #" + std::to_string(i), misc::LogMessageType::exclamation);
            return;
        }
        if (desc.SystemValueType != D3D_NAME_UNDEFINED) 
        {
            continue;
        }

        ShaderArgumentInfoKey arg_key{ 
            .semantic_name = std::string{desc.SemanticName, strlen(desc.SemanticName)},
            .semantic_index = static_cast<uint32_t>(desc.SemanticIndex) 
        };
        ShaderArgumentInfo arg_info{};
        arg_info.kind = kind;
        arg_info.register_index = static_cast<uint32_t>(desc.Register);

        bool is_fp{}, is_signed{};
        unsigned char element_count{}, element_size{};
        switch (desc.ComponentType)
        {
        // case D3D_REGISTER_COMPONENT_UINT64:
        case D3D_REGISTER_COMPONENT_UINT32:
            is_fp = false;
            is_signed = false;
            element_size = 4;
            break;

        // case D3D_REGISTER_COMPONENT_SINT64:
        case D3D_REGISTER_COMPONENT_SINT32:
            is_fp = false;
            is_signed = true;
            element_size = 4;
            break;

        // case D3D_REGISTER_COMPONENT_FLOAT64:
        case D3D_REGISTER_COMPONENT_FLOAT32:
            is_fp = true;
            is_signed = true;
            element_size = 4;
            break;

        /*case D3D_REGISTER_COMPONENT_UINT16:
            is_fp = false;
            is_signed = false;
            element_size = 2;
            break;

        case D3D_REGISTER_COMPONENT_SINT16:
            is_fp = false;
            is_signed = true;
            element_size = 2;
            break;

        case D3D_REGISTER_COMPONENT_FLOAT16:
            is_fp = true;
            is_signed = true;
            element_size = 2;
            break;*/
        
        default:
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "Unsupported shader input parameter component type");
        }

        switch (desc.Mask)
        {
        case 1:
            element_count = 1;
            break;

        case 0b11:
            element_count = 2;
            break;

        case 0b111:
            element_count = 3;
            break;

        case 0b1111:
            element_count = 4;
            break;

        default:
            LEXGINE_ASSUME;
        }

        dx::d3d12::DxResourceFactory const* p_dx_resource_factory = m_globals.get<dx::d3d12::DxResourceFactory>();
        arg_info.format = p_dx_resource_factory->dxgiFormatFetcher().fetch(is_fp, is_signed, false, element_count, element_size);

        p_target_map->insert(std::make_pair(arg_key, arg_info));
    }
}

//void ShaderStage::fillDescriptorTableRanges(d3d12::RootEntryDescriptorTable& target_descriptor_table, d3d12::RootEntryDescriptorTable::RangeType range_type)
//{
//    BindingDesc const* p_first_binding_desc = nullptr;
//    size_t binding_descs_count{};
//    d3d12::StaticDescriptorHeapAllocationManager::UPointer const* p_first_bound_resource = nullptr;
//
//    switch (range_type)
//    {
//    case lexgine::core::dx::d3d12::RootEntryDescriptorTable::RangeType::srv:
//        binding_descs_count = m_texture_binding_descs.size();
//        p_first_binding_desc = binding_descs_count > 0 ? &m_texture_binding_descs[0] : nullptr;
//        p_first_bound_resource = binding_descs_count > 0 ? &m_bound_srv_resources[0] : nullptr;
//        break;
//    case lexgine::core::dx::d3d12::RootEntryDescriptorTable::RangeType::uav:
//        binding_descs_count = m_storage_buffer_binding_descs.size();
//        p_first_binding_desc = binding_descs_count > 0 ? &m_storage_buffer_binding_descs[0] : nullptr;
//        p_first_bound_resource = binding_descs_count > 0 ? &m_bound_uav_resources[0] : nullptr;
//        break;
//    case lexgine::core::dx::d3d12::RootEntryDescriptorTable::RangeType::cbv:
//        binding_descs_count = m_cbuffer_binding_descs.size();
//        p_first_binding_desc = binding_descs_count > 0 ? &m_cbuffer_binding_descs[0] : nullptr;
//        p_first_bound_resource = binding_descs_count > 0 ? &m_bound_cbv_resources[0] : nullptr;
//        break;
//    case lexgine::core::dx::d3d12::RootEntryDescriptorTable::RangeType::sampler:
//        // TODO: implement support for sampler bindings
//        break;
//    default:
//        LEXGINE_ASSUME;
//    }
//
//    for (size_t i = 0; i < binding_descs_count; ++i)
//    {
//        target_descriptor_table.addRange(range_type, p_first_binding_desc[i].binding_points_count, p_first_binding_desc[i].binding_point,
//            p_first_binding_desc[i].register_space, p_first_bound_resource[p_first_binding_desc[i].bound_resource_id].cpu_pointer);
//    }
//}



} // namespace lexgine::core::dx::dxcompilation


