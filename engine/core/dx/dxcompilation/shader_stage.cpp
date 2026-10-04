#include <chrono>
#include <thread>
#include <tuple>

#include "engine/core/exception.h"
#include "engine/core/globals.h"
#include "engine/core/dx/d3d12/d3d_data_blob.h"
#include "engine/core/misc/log.h"
#include "engine/core/misc/strict_weak_ordering.h"
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

void ShaderStage::build()
{
    if (m_is_ready)
    {
        return;
    }

    auto [shader_blob, shader_status] = m_shader_blob_cache.getShaderBlob(m_shader_handle);
    if (shader_status == d3d12::caches::HLSLShaderBlobCompilationStatus::Started)
    {
        misc::Log::retrieve()->out("Unable to create reflection for shader: shader compilation is not completed yet, waiting for completion", misc::LogMessageType::exclamation);
        unsigned int reps = 0;
        while (shader_status == d3d12::caches::HLSLShaderBlobCompilationStatus::Started && reps < 60)
        {
            std::this_thread::sleep_for(std::chrono::seconds { 1 });
            std::tie(shader_blob, shader_status) = m_shader_blob_cache.getShaderBlob(m_shader_handle);
            ++reps;
        }
        if (shader_status == d3d12::caches::HLSLShaderBlobCompilationStatus::Started)
        {
            misc::Log::retrieve()->out("Unable to complete compilation of HLSL shader '" + m_shader_name + "': timeout", misc::LogMessageType::error);
            return;
        }
    }
    if (shader_status != d3d12::caches::HLSLShaderBlobCompilationStatus::Completed || !shader_blob)
    {
        misc::Log::retrieve()->out("Unable to create reflection for shader '" + m_shader_name + "': shader compilation failed", misc::LogMessageType::exclamation);
        return;
    }

    LEXGINE_LOG_ERROR_IF_FAILED(this, DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_dxc_utils.GetAddressOf())), S_OK);
    if (getErrorState()) {
        misc::Log::retrieve()->out("Unable to create reflection for shader '" + m_shader_name + "': DxcUtils creation failed", misc::LogMessageType::exclamation);
        return;
    }

    DxcBuffer hlsl_source_dxc_buffer { .Ptr = shader_blob.data(), .Size = shader_blob.size(), .Encoding = 0 };
    LEXGINE_LOG_ERROR_IF_FAILED(this, m_dxc_utils->CreateReflection(&hlsl_source_dxc_buffer, IID_PPV_ARGS(m_shader_reflection.GetAddressOf())), S_OK);
    if (getErrorState()) {
        misc::Log::retrieve()->out("Unable to create reflection for shader '" + m_shader_name + "': shader reflection creation failed", misc::LogMessageType::exclamation);
        return;
    }

    LEXGINE_LOG_ERROR_IF_FAILED(this, m_shader_reflection->GetDesc(&m_shader_desc), S_OK);
    if (getErrorState()) {
        misc::Log::retrieve()->out("Unable to create reflection for shader '" + m_shader_name + "': shader reflection description retrieval failed", misc::LogMessageType::exclamation);
        return;
    }

    collectShaderArguments(ShaderArgumentKind::input);
    collectShaderArguments(ShaderArgumentKind::output);
    collectShaderBindings();

    m_is_ready = true;
}

unsigned int ShaderStage::getInstructionCount()
{
    return static_cast<unsigned int>(m_shader_desc.InstructionCount);
}

lexgine::core::dx::d3d12::D3DDataBlob ShaderStage::getShaderBytecode() const
{
    return m_shader_blob_cache.getShaderBlob(m_shader_handle).first;
}

d3d12::ConstantBufferReflection ShaderStage::buildConstantBufferReflection(misc::HashedString const& constant_buffer_name) const
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
    return m_shader_blob_cache.getShaderType(m_shader_handle);
}

ShaderModel ShaderStage::getShaderModel() const
{
    return m_shader_blob_cache.getShaderModel(m_shader_handle);
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

ShaderStage::ShaderStage(Globals const& globals, d3d12::caches::HLSLShaderHandle shader_handle, ShaderFunction* p_owning_shader_function)
    : m_globals{ globals }
    , m_shader_blob_cache{ globals.hlslShaderBlobCache() }
    , m_shader_handle{ shader_handle }
    , m_owning_shader_function_ptr{ p_owning_shader_function }
    , m_shader_name { m_shader_blob_cache.getShaderCacheName(shader_handle) }
{
    
}

void ShaderStage::collectShaderBindings()
{
    m_shader_resource_names_pool.clear();
    m_texture_shader_inputs.clear();
    m_storage_block_shader_inputs.clear();

    for (UINT i = 0; i < m_shader_desc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC desc{};
        m_shader_reflection->GetResourceBindingDesc(i, &desc);

        misc::HashedString hashed_name{ desc.Name };
        assert(!m_shader_resource_names_pool.contains(hashed_name));

        ShaderFunction::ShaderBindingPoint binding_point{};
        binding_point.is_unbounded = desc.BindCount == 0;
        binding_point.first_register = static_cast<uint32_t>(desc.BindPoint);
        binding_point.register_count = binding_point.is_unbounded ? 0 : static_cast<uint32_t>(desc.BindCount);
        binding_point.register_space = static_cast<uint32_t>(desc.Space);

        switch (desc.Type)
        {
        case D3D_SIT_CBUFFER:
            binding_point.kind = ShaderFunction::ShaderInputKind::cbv;
            break;

        case D3D_SIT_SAMPLER:
            binding_point.kind = ShaderFunction::ShaderInputKind::sampler;
            if (desc.uFlags & D3D_SIF_COMPARISON_SAMPLER)
            {
                binding_point.kind = ShaderFunction::ShaderInputKind::comparison_sampler;
            }
            break;

        case D3D_SIT_TEXTURE:
        case D3D_SIT_TBUFFER:
        case D3D_SIT_STRUCTURED:
        case D3D_SIT_BYTEADDRESS:
        {
            binding_point.kind = ShaderFunction::ShaderInputKind::srv;
            TextureShaderInputInfo extra_info{};
            extra_info.ms_count = static_cast<uint32_t>(desc.NumSamples);
            extra_info.data_type = static_cast<StorageResourceDataType>(desc.ReturnType);
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
            StorageBlockShaderInputInfo extra_info{};
            extra_info.data_type = static_cast<StorageResourceDataType>(desc.ReturnType);
            m_storage_block_shader_inputs[binding_point] = extra_info;
            break;
        }

        case D3D_SIT_UAV_FEEDBACKTEXTURE:
        {
            LEXGINE_LOG_ERROR(
                this,
                std::format("Sampler feedback texture binding was "
                    "encountered at register range {}{}..{}. Sampler feedback textures are not currently supported. "
                    "This shader input will be ignored and attempts to execute it will result in undefined behaviour",
                    'u', binding_point.first_register, binding_point.first_register + binding_point.register_count - 1
                )
            );
            continue;
        }

        case D3D_SIT_RTACCELERATIONSTRUCTURE:
        {
            LEXGINE_LOG_ERROR(
                this,
                std::format("Ray-tracing acceleration structure binding was "
                    "encountered at register range {}{}..{}. Ray-tracing is not currently supported. "
                    "This shader input will be ignored and attempts to execute it will result in undefined behaviour",
                    't', binding_point.first_register, binding_point.first_register + binding_point.register_count - 1
                )
            );
            continue;
        }

        default:
            LEXGINE_ASSUME;
        }
        m_shader_resource_names_pool.insert(std::make_pair(hashed_name, binding_point));

        switch (binding_point.kind)
        {
        case ShaderFunction::ShaderInputKind::cbv:
        case ShaderFunction::ShaderInputKind::sampler:
        case ShaderFunction::ShaderInputKind::comparison_sampler:
            break;

        case ShaderFunction::ShaderInputKind::srv:
        {
            TextureShaderInputInfo& info = m_texture_shader_inputs[binding_point];
            switch (desc.Type)
            {
            case D3D_SIT_TEXTURE:
            {
                switch (desc.Dimension)
                {
                case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
                    info.is_array = true;
                    [[fallthrough]];
                case D3D_SRV_DIMENSION_TEXTURE1D:
                    info.resource_type = TextureResourceType::resource_with_dimension;
                    info.dimension = ResourceDimension::texture1d;
                    break;

                case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
                    info.is_array = true;
                    [[fallthrough]];
                case D3D_SRV_DIMENSION_TEXTURE2DMS:
                    info.is_ms = true;
                    info.resource_type = TextureResourceType::resource_with_dimension;
                    info.dimension = ResourceDimension::texture2d;
                    break;

                case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
                    info.is_array = true;
                    [[fallthrough]];
                case D3D_SRV_DIMENSION_TEXTURE2D:
                    info.resource_type = TextureResourceType::resource_with_dimension;
                    info.dimension = ResourceDimension::texture2d;
                    break;
               
                case D3D_SRV_DIMENSION_TEXTURE3D:
                    info.resource_type = TextureResourceType::resource_with_dimension;
                    info.dimension = ResourceDimension::texture3d;
                    break;

                case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
                    info.is_array = true;
                    [[fallthrough]];
                case D3D_SRV_DIMENSION_TEXTURECUBE:
                    info.is_cube = true;
                    info.resource_type = TextureResourceType::resource_with_dimension;
                    info.dimension = ResourceDimension::texture2d;
                    break;

                case D3D_SRV_DIMENSION_BUFFER:
                    info.resource_type = TextureResourceType::resource_with_dimension;
                    info.dimension = ResourceDimension::buffer;
                    info.is_buffer = true;
                    break;

                default:
                    LEXGINE_ASSUME;
                }
                break;
            }

            case D3D_SIT_TBUFFER:
                info.resource_type = TextureResourceType::tbuffer;
                info.is_buffer = true;
                break;

            case D3D_SIT_STRUCTURED:
                info.resource_type = TextureResourceType::structured_buffer;
                info.is_buffer = true;
                break;

            case D3D_SIT_BYTEADDRESS:
                info.resource_type = TextureResourceType::raw_buffer;
                info.is_buffer = true;
                break;

            default:
                LEXGINE_ASSUME;
            }
            break;
        }


        case ShaderFunction::ShaderInputKind::uav:
        {
            StorageBlockShaderInputInfo& info = m_storage_block_shader_inputs[binding_point];
            switch (desc.Type)
            {
            case D3D_SIT_UAV_RWTYPED:
                switch (desc.Dimension)
                {
                case D3D_SRV_DIMENSION_BUFFER:
                    info.resource_type = StorageBlockResourceType::resource_with_dimension;
                    info.dimension = ResourceDimension::buffer;
                    info.is_buffer = true;
                    break;

                case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
                    info.is_array = true;
                    [[fallthrough]];
                case D3D_SRV_DIMENSION_TEXTURE1D:
                    info.resource_type = StorageBlockResourceType::resource_with_dimension;
                    info.dimension = ResourceDimension::texture1d;
                    break;

                case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
                    info.is_array = true;
                    [[fallthrough]];
                case D3D_SRV_DIMENSION_TEXTURE2DMS:
                    info.is_ms = true;
                    info.resource_type = StorageBlockResourceType::resource_with_dimension;
                    info.dimension = ResourceDimension::texture2d;
                    break;

                case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
                    info.is_array = true;
                    [[fallthrough]];
                case D3D_SRV_DIMENSION_TEXTURE2D:
                    info.resource_type = StorageBlockResourceType::resource_with_dimension;
                    info.dimension = ResourceDimension::texture2d;
                    break;

                case D3D_SRV_DIMENSION_TEXTURE3D:
                    info.resource_type = StorageBlockResourceType::resource_with_dimension;
                    info.dimension = ResourceDimension::texture3d;
                    break;

                default:
                    LEXGINE_LOG_ERROR(
                        this,
                        std::format("Typed UAV binding '{}' at register u{} has unsupported dimension {}",
                            desc.Name, binding_point.first_register, static_cast<int>(desc.Dimension)
                        )
                    );
                    break;
                }
                break;

            case D3D_SIT_UAV_RWSTRUCTURED:
                info.resource_type = StorageBlockResourceType::structured_buffer;
                info.is_buffer = true;
                break;

            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                info.resource_type = StorageBlockResourceType::structured_buffer_with_counter;
                info.is_buffer = true;
                break;

            case D3D_SIT_UAV_RWBYTEADDRESS:
                info.resource_type = StorageBlockResourceType::raw_buffer;
                info.is_buffer = true;
                break;

            case D3D_SIT_UAV_APPEND_STRUCTURED:
                info.resource_type = StorageBlockResourceType::append_structured_buffer;
                info.is_buffer = true;
                break;

            case D3D_SIT_UAV_CONSUME_STRUCTURED:
                info.resource_type = StorageBlockResourceType::consume_structured_buffer;
                info.is_buffer = true;
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

    p_target_map->clear();
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

        dx::d3d12::DxResourceFactory const& dx_resource_factory = m_globals.dxResourceFactory();
        arg_info.format = dx_resource_factory.dxgiFormatFetcher().fetch(is_fp, is_signed, false, element_count, element_size);
        if (arg_info.format == DXGI_FORMAT_UNKNOWN)
        {
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(
                this,
                std::format("Unable to find requested support format with the following properties: fp={}, signed={}, normalized={}, element_count={}, element_size={}",
                    is_fp, is_signed, false, element_count, element_size)
            );
        }
        p_target_map->insert(std::make_pair(arg_key, arg_info));
    }
}

std::vector<ReflectedDeclaration> ShaderStage::reflectedDeclarations() const
{
    std::vector<ReflectedDeclaration> declarations;
    declarations.reserve(m_shader_resource_names_pool.size());
    for (auto const& [name, binding_point] : m_shader_resource_names_pool)
    {
        ViewRequirements view{};
        switch (binding_point.kind)
        {
        case ShaderInputKind::srv:
            view = m_texture_shader_inputs.at(binding_point);
            break;
        case ShaderInputKind::uav:
            view = m_storage_block_shader_inputs.at(binding_point);
            break;
        default:
            break;
        }

        declarations.push_back(ReflectedDeclaration{ .stage = getShaderType(), .name = name, .binding = binding_point, .view = view });
    }

    return declarations;
}



} // namespace lexgine::core::dx::dxcompilation


