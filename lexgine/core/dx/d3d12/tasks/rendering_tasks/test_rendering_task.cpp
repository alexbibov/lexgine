#include <chrono>

#include "test_rendering_task.h"

#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/math/utility.h"
#include "lexgine/core/dx/d3d12/device.h"
#include "lexgine/core/dx/d3d12/basic_rendering_services.h"
#include "lexgine/core/dx/d3d12/task_caches/root_signature_compilation_task_cache.h"
#include "lexgine/core/dx/d3d12/tasks/root_signature_compilation_task.h"
#include "lexgine/core/dx/d3d12/tasks/hlsl_compilation_task.h"
#include "lexgine/core/dx/d3d12/tasks/pso_compilation_task.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;
using namespace lexgine::core::dx::d3d12::task_caches;

TestRenderingTask::TestRenderingTask(Globals& globals, BasicRenderingServices& rendering_services)
    : m_globals{ globals }
    , m_device{ *globals.get<Device>() }
    , m_basic_rendering_services{ rendering_services }
    , m_data_uploader{ globals, rendering_services.resourceUploadAllocator() }
    , m_vb{ m_device }
    , m_ib{ m_device, IndexDataType::_16_bit, 32 * 1024 }
    , m_texture{ m_device, ResourceState::enum_type::pixel_shader, misc::makeEmptyOptional<ResourceOptimizedClearValue>(),
                ResourceDescriptor::CreateTexture2D(256, 256, 1, DXGI_FORMAT_R8G8B8A8_UNORM), AbstractHeapType::default,
                HeapCreationFlags::enum_type::allow_all }
    , m_cb_data_mapping{ m_cb_reflection }
    , m_timestamp{ std::chrono::high_resolution_clock::now().time_since_epoch().count() }
    , m_cmd_list{ m_device.createCommandList(CommandType::direct, 0x1) }
    , m_allocation{ nullptr }
{
    std::shared_ptr<AbstractVertexAttributeSpecification> position = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
        std::make_shared<VertexAttributeSpecification<float, 3>>(0, 0, "POSITION", 0, 0));
    std::shared_ptr<AbstractVertexAttributeSpecification> color = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
        std::make_shared<VertexAttributeSpecification<float, 3>>(0, 12, "COLOR", 0, 0));
    std::shared_ptr<AbstractVertexAttributeSpecification> texture_coordinate = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
        std::make_shared<VertexAttributeSpecification<float, 2>>(0, 24, "TEXCOORD", 0, 0));

    m_va_list = VertexAttributeSpecificationList{ position, color, texture_coordinate };

    {
        // upload vertex buffer

        m_vb.setSegment(m_va_list, 8, 0);
        m_vb.build();

        ResourceDataUploader::DestinationDescriptor destination_descriptor;
        destination_descriptor.p_destination_resource = &m_vb.resource();
        destination_descriptor.destination_resource_state = ResourceState::enum_type::vertex_and_constant_buffer;
        destination_descriptor.segment.base_offset = 0U;

        m_box_vertices = std::array<float, 64>{
            -1.f, -1.f, -1.f, 1.f, 0.f, 0.f, 0.f, 0.f,
                1.f, -1.f, -1.f, 0.f, 1.f, 0.f, 1.f, 0.f,
                1.f, 1.f, -1.f, 0.f, 0.f, 1.f, 1.f, 1.f,
                -1.f, 1.f, -1.f, .5f, .5f, .5f, 0.f, 1.f,

                -1.f, -1.f, 1.f, .5f, .5f, .5f, 1.f, 1.f,
                1.f, -1.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f,
                1.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f,
                -1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f
        };

        ResourceDataUploader::BufferSourceDescriptor source_descriptor;
        source_descriptor.p_data = m_box_vertices.data();
        source_descriptor.buffer_size = m_box_vertices.size() * sizeof(float);

        m_data_uploader.addResourceForUpload(destination_descriptor, source_descriptor);
    }


    {
        // upload index buffer

        ResourceDataUploader::DestinationDescriptor destination_descriptor;
        destination_descriptor.p_destination_resource = &m_ib.resource();
        destination_descriptor.destination_resource_state = m_ib.defaultState();
        destination_descriptor.segment.base_offset = 0U;

        m_box_indices = std::array<short, 36>{
            0, 3, 1, 1, 3, 2,
                4, 5, 7, 5, 6, 7,
                7, 6, 3, 6, 2, 3,
                4, 0, 5, 5, 0, 1,
                5, 1, 6, 1, 2, 6,
                0, 4, 7, 0, 7, 3
        };

        ResourceDataUploader::BufferSourceDescriptor source_descriptor;
        source_descriptor.p_data = m_box_indices.data();
        source_descriptor.buffer_size = m_box_indices.size() * sizeof(short);

        m_data_uploader.addResourceForUpload(destination_descriptor, source_descriptor);
    }

    {
        // upload texture
        std::vector<unsigned char> texture_data((1 << 16) * 4, 0);
        for (int i = 0; i < 256; ++i)
            for (int j = 0; j < 256; ++j)
                texture_data[4 * (i * 256ULL + j)] = static_cast<unsigned char>(((i & j & 0x8) >> 3) * 255);

        ResourceDataUploader::TextureSourceDescriptor source_descriptor;
        source_descriptor.subresources.push_back({ texture_data.data(), 256 * 4, 256 * 256 * 4 });

        ResourceDataUploader::DestinationDescriptor destination_descriptor;
        destination_descriptor.destination_resource_state = m_texture.defaultState();
        destination_descriptor.p_destination_resource = &m_texture;
        destination_descriptor.segment.subresources.first_subresource = 0;
        destination_descriptor.segment.subresources.num_subresources = 1;

        m_data_uploader.addResourceForUpload(destination_descriptor, source_descriptor);
    }

    m_data_uploader.upload();
    m_data_uploader.waitUntilUploadIsFinished();

    // Create root signature and the related descriptor tables
    {
        RootSignatureCompilationTaskCache& rs_compilation_task_cache = *globals.get<RootSignatureCompilationTaskCache>();
        RootSignature rs{};

        RootEntryDescriptorTable main_parameters;
        main_parameters.addRange(RootEntryDescriptorTable::RangeType::srv, 1, 0, 0, 0);

        RootEntryDescriptorTable sampler_table;
        sampler_table.addRange(RootEntryDescriptorTable::RangeType::sampler, 1, 0, 0, 0);

        rs.addParameter(0, main_parameters);
        rs.addParameter(1, sampler_table);
        rs.addParameter(2, RootEntryCBVDescriptor{ 0, 0 });

        RootSignatureFlags rs_flags = RootSignatureFlags::enum_type::allow_input_assembler;
        // rs_flags |= RootSignatureFlags::enum_type::deny_domain_shader;
        // rs_flags |= RootSignatureFlags::enum_type::deny_hull_shader;

        m_rs = rs_compilation_task_cache.findOrCreateTask(globals, std::move(rs), rs_flags, "test_rendering_rs", 0);
        m_rs->execute(0);

        {
            ResourceViewDescriptorTableBuilder builder{ globals, 0 };
            builder.addDescriptor(SRVDescriptor{ m_texture, SRVTextureInfo{} });
            m_srv_table = builder.build();
        }

        {
            FilterPack filter_pack{ MinificationFilter::linear, MagnificationFilter::linear, 16,
                WrapMode::clamp, WrapMode::clamp, WrapMode::clamp };

            SamplerDescriptorTableBuilder builder{ globals, 0 };
            builder.addDescriptor(SamplerDescriptor{ filter_pack, math::Vector4f{0.f} });
            m_sampler_table = builder.build();
        }
    }


    // Create shaders
    {
        std::string hlsl_source =
            "struct ConstantDataStruct\n"
            "{\n"
            "    float4x4 ProjectionMatrix;\n"
            "    float rotationAngle;\n"
            "};\n"
            "\n"
            "ConstantBuffer<ConstantDataStruct> ConstantData : register(b0);\n"
            "Texture2D<float4> SampleTexture : register(t0);\n"
            "SamplerState BillinearSampler : register(s0);\n"
            "\n"
            "struct SOutput\n"
            "{\n"
            "    float4 transformed_position: SV_POSITION;\n"
            "    float3 color: COLOR;\n"
            "    float2 texcoord: TEXCOORD;\n"
            "};\n"
            "struct SInput\n"
            "{\n"
            "    float3 position: POSITION;\n"
            "    float3 color: COLOR;\n"
            "    float2 texcoord: TEXCOORD;\n"
            "};\n"
            "\n"
            "SOutput VSMain(SInput s_input)\n"
            "{\n"
            "    float4x4 view = float4x4(1.f, 0.f, 0.f, 0.f, \n"
            "                             0.f, 1.f, 0.f, 0.f, \n"
            "                             0.f, 0.f, 1.f, -3.f, \n"
            "                             0.f, 0.f, 0.f, 1.f);\n"
            "    float4x4 model = float4x4(cos(ConstantData.rotationAngle), 0.f, sin(ConstantData.rotationAngle), 0.f, \n"
            "                              0.f, 1.f, 0.f, 0.f, \n"
            "                              -sin(ConstantData.rotationAngle), 0.f, cos(ConstantData.rotationAngle), 0.f, \n"
            "                              0.f, 0.f, 0.f, 1.f);\n"
            "    float4x4 mvp = mul(ConstantData.ProjectionMatrix, mul(view, model));\n"
            "    SOutput s_output;\n"
            "    s_output.transformed_position = mul(mvp, float4(s_input.position, 1.f));\n"
            "    s_output.color = s_input.color;\n"
            "    s_output.texcoord = s_input.texcoord;\n"
            "    return s_output;\n"
            "}\n"
            "\n"
            "\n"
            "float4 PSMain(SOutput s_output): SV_Target0\n"
            "{\n"
            "    float3 color = s_output.color + SampleTexture.Sample(BillinearSampler, s_output.texcoord).r;\n"
            "    color = 1.f - exp(-color);\n"
            "    return float4(color, 1.f);\n"
            "}\n"
            "\n";

        HLSLCompilationTaskCache& hlsl_compilation_task_cache = *globals.get<HLSLCompilationTaskCache>();

        HLSLSourceTranslationUnit hlsl_translation_unit{ globals, "test_rendering_shader", hlsl_source };

        m_vs = hlsl_compilation_task_cache.findOrCreateTask(hlsl_translation_unit,
            dxcompilation::ShaderModel::model_60, dxcompilation::ShaderType::vertex, "VSMain");

        m_ps = hlsl_compilation_task_cache.findOrCreateTask(hlsl_translation_unit,
            dxcompilation::ShaderModel::model_60, dxcompilation::ShaderType::pixel, "PSMain");

        m_vs->execute(0);
        m_ps->execute(0);
    }

    // Setup constant buffer data
    {
        m_cb_reflection.addElement("ProjectionMatrix",
            ConstantBufferReflection::ReflectionEntryDesc{ ConstantBufferReflection::ReflectionEntryBaseType::float4x4, 1 });

        m_cb_reflection.addElement("RotationAngle",
            ConstantBufferReflection::ReflectionEntryDesc{ ConstantBufferReflection::ReflectionEntryBaseType::float1, 1 });

        m_cb_data_mapping.addDataBinding("ProjectionMatrix", m_projection_transform);
        m_cb_data_mapping.addDataBinding("RotationAngle", m_box_rotation_angle);

        m_projection_transform = math::createProjectionMatrix(misc::EngineAPI::Direct3D12, 16.f, 9.f);
    }
}

void TestRenderingTask::updateBufferFormats(DXGI_FORMAT color_target_format, DXGI_FORMAT depth_target_format)
{
    PSOCompilationTaskCache& pso_compilation_task_cache = *m_globals.get<PSOCompilationTaskCache>();

    GraphicsPSODescriptor pso_descriptor{};
    pso_descriptor.vertex_attributes = m_va_list;
    pso_descriptor.rtv_formats[0] = color_target_format;
    pso_descriptor.num_render_targets = 1;
    pso_descriptor.dsv_format = depth_target_format;
    pso_descriptor.primitive_topology_type = PrimitiveTopologyType::triangle;
    pso_descriptor.node_mask = 1;
    pso_descriptor.primitive_restart = true;

    m_pso = pso_compilation_task_cache.findOrCreateTask(m_globals, pso_descriptor,
        "test_rendering_pso_" + std::to_string(color_target_format) + "_" + std::to_string(depth_target_format), 0);
    m_pso->setVertexShaderCompilationTask(m_vs);
    m_pso->setPixelShaderCompilationTask(m_ps);
    m_pso->setRootSignatureCompilationTask(m_rs);

    m_pso->execute(0);
}

bool TestRenderingTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    m_cmd_list.reset();

    m_basic_rendering_services.setDefaultResources(m_cmd_list);
    m_basic_rendering_services.beginRendering(m_cmd_list);

    m_cmd_list.setPipelineState(m_pso->getTaskData());

    m_cmd_list.setRootSignature(m_rs->getCacheName());
    m_basic_rendering_services.setDefaultViewport(m_cmd_list);


    m_cmd_list.inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle_list);
    m_vb.bind(m_cmd_list);
    m_ib.bind(m_cmd_list);

    m_basic_rendering_services.setDefaultRenderingTarget(m_cmd_list);
    m_basic_rendering_services.clearDefaultRenderingTarget(m_cmd_list);

    long long new_timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    float duration = (new_timestamp - m_timestamp) / 1e9f;
    m_box_rotation_angle += static_cast<float>(math::pi * duration * .5f);
    m_timestamp = new_timestamp;

    m_allocation =
        m_basic_rendering_services.constantDataStream().allocateAndUpdate(m_cb_data_mapping);
     
    m_cmd_list.setRootDescriptorTable(0, m_srv_table);
    m_cmd_list.setRootDescriptorTable(1, m_sampler_table);
    m_cmd_list.setRootConstantBufferView(2, m_allocation->virtualGpuAddress());

    m_cmd_list.drawIndexedInstanced(36, 1, 0, 0, 0);

    // m_basic_rendering_services.endRendering(m_cmd_list);

    m_cmd_list.close();

    m_device.defaultCommandQueue().executeCommandList(m_cmd_list);

    return true;
}
