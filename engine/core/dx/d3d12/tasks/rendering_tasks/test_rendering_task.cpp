#include <chrono>

#include "engine/core/globals.h"
#include "engine/core/engine_api.h"
#include "engine/core/global_settings.h"
#include "engine/core/profiling_services.h"
#include "engine/core/rendering_configuration.h"
#include "engine/core/math/utility.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/basic_rendering_services.h"
#include "engine/core/dx/d3d12/task_caches/root_signature_compilation_task_cache.h"
#include "engine/core/dx/d3d12/tasks/root_signature_compilation_task.h"
#include "engine/core/dx/d3d12/tasks/hlsl_compilation_task.h"
#include "engine/core/dx/d3d12/tasks/pso_compilation_task.h"

#include "engine/core/dx/dxcompilation/shader_stage.h"
#include "engine/core/dx/dxcompilation/shader_function.h"

#include "test_rendering_task.h"


using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;
using namespace lexgine::core::dx::d3d12::task_caches;


TestRenderingTask::TestRenderingTask(Globals& globals, BasicRenderingServices& rendering_services)
    : RenderingWork{ globals, "Test rendering task", CommandType::direct }
    , m_globals{ globals }
    , m_device{ *globals.get<Device>() }
    , m_basic_rendering_services{ rendering_services }
    , m_vb{ m_device }
    , m_ib{ m_device, IndexDataType::_16_bit, 32 * 1024 }
    , m_texture{ m_device, ResourceState::base_values::pixel_shader, misc::makeEmptyOptional<ResourceOptimizedClearValue>(),
                ResourceDescriptor::CreateTexture2D(256, 256, 1, DXGI_FORMAT_R8G8B8A8_UNORM), AbstractHeapType::_default,
                HeapCreationFlags::base_values::allow_all }
    , m_cb_data_mapping{ m_cb_reflection }
    , m_timestamp{ std::chrono::high_resolution_clock::now().time_since_epoch().count() }
    , m_box_rotation_angle{ 0.f }
    , m_allocation{ nullptr }
    , m_cmd_list_ptr{ addCommandList() }
{
    std::shared_ptr<AbstractVertexAttributeSpecification> position = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
        std::make_shared<VertexAttributeSpecification<float, 3>>(0, 0, "POSITION", 0, 0));
    std::shared_ptr<AbstractVertexAttributeSpecification> color = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
        std::make_shared<VertexAttributeSpecification<float, 3>>(0, 12, "COLOR", 0, 0));
    std::shared_ptr<AbstractVertexAttributeSpecification> texture_coordinate = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
        std::make_shared<VertexAttributeSpecification<float, 2>>(0, 24, "TEXCOORD", 0, 0));

    m_va_list = VertexAttributeSpecificationList{ position, color, texture_coordinate };
    auto& data_uploader = m_basic_rendering_services.resourceDataUploader();

    m_projection_transform = math::createProjectionMatrix(EngineApi::Direct3D12, 16.f, 9.f);

    {
        // upload vertex buffer

        m_vb.setSegment(m_va_list, 8, 0);
        m_vb.build();

        ResourceDataUploader::DestinationDescriptor destination_descriptor;
        destination_descriptor.p_destination_resource = &m_vb.resource();
        destination_descriptor.destination_resource_state = ResourceState::base_values::vertex_and_constant_buffer;
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

        data_uploader.addResourceForUpload(destination_descriptor, source_descriptor);
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

        data_uploader.addResourceForUpload(destination_descriptor, source_descriptor);
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

        data_uploader.addResourceForUpload(destination_descriptor, source_descriptor);
    }

    data_uploader.upload();
    data_uploader.waitUntilUploadIsFinished();


    // Create shaders
    {
        std::string hlsl_source =
            "struct ConstantDataStruct\n"
            "{\n"
            "    float4x4 ProjectionMatrix;\n"
            "    float rotationAngle;\n"
            "};\n"
            "\n"
            "ConstantBuffer<ConstantDataStruct> ConstantData : register(b0, space100);\n"
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

        dxcompilation::ShaderFunction shader_function{ m_globals, 0, "test_rendering_shader" };
        dxcompilation::ShaderStage* p_vs_stage = shader_function.createShaderStage(m_vs);
        dxcompilation::ShaderStage* p_ps_stage = shader_function.createShaderStage(m_ps);

        p_ps_stage->bindTexture(std::string{ "SampleTexture" }, m_texture);
        p_ps_stage->bindSampler(std::string{ "BillinearSampler" },
            FilterPack{ MinificationFilter::linear, MagnificationFilter::linear, 16, WrapMode::clamp, WrapMode::clamp, WrapMode::clamp }, 
            math::Vector4f{ 0.f });

        shader_function.prepare(false);
        m_rs = shader_function.getBindingSignature();
         
        m_cb_reflection = p_vs_stage->buildConstantBufferReflection(std::string{ "ConstantData" });
        m_cb_data_mapping.addDataBinding("ProjectionMatrix", m_projection_transform);
        m_cb_data_mapping.addDataBinding("rotationAngle", m_box_rotation_angle);
    }
}

void TestRenderingTask::updateRenderingConfiguration(RenderingConfigurationUpdateFlags update_flags, RenderingConfiguration const& rendering_configuration)
{
    if (update_flags.isSet(RenderingConfigurationUpdateFlags::base_values::color_format_changed)
        || update_flags.isSet(RenderingConfigurationUpdateFlags::base_values::depth_format_changed))
    {
        PSOCompilationTaskCache& pso_compilation_task_cache = *m_globals.get<PSOCompilationTaskCache>();

        GraphicsPSODescriptor pso_descriptor{};
        pso_descriptor.vertex_attributes = m_va_list;
        pso_descriptor.rtv_formats[0] = rendering_configuration.color_buffer_format;
        pso_descriptor.num_render_targets = 1;
        pso_descriptor.dsv_format = rendering_configuration.depth_buffer_format;
        pso_descriptor.primitive_topology_type = PrimitiveTopologyType::triangle;
        pso_descriptor.node_mask = 1;
        pso_descriptor.primitive_restart = true;

        m_pso = pso_compilation_task_cache.findOrCreateTask(m_globals, pso_descriptor,
            "test_rendering_pso_" + std::to_string(rendering_configuration.color_buffer_format)
            + "_" + std::to_string(rendering_configuration.depth_buffer_format), 0);
        m_pso->setVertexShaderCompilationTask(m_vs);
        m_pso->setPixelShaderCompilationTask(m_ps);
        m_pso->setRootSignatureCompilationTask(m_rs);

        m_pso->execute(0);
    }
}

bool TestRenderingTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    //cmd_list.reset();

    m_basic_rendering_services.beginRendering(*m_cmd_list_ptr);
    
    m_cmd_list_ptr->setPipelineState(m_pso->getTaskData());
    m_cmd_list_ptr->setRootSignature(m_rs->getCacheName());

    m_basic_rendering_services.setDefaultResources(*m_cmd_list_ptr);
    
    m_basic_rendering_services.setDefaultViewport(*m_cmd_list_ptr);

    m_cmd_list_ptr->inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle_list);
    m_vb.bind(*m_cmd_list_ptr);
    m_ib.bind(*m_cmd_list_ptr);

    m_basic_rendering_services.setDefaultRenderingTarget(*m_cmd_list_ptr);
    m_basic_rendering_services.clearDefaultRenderingTarget(*m_cmd_list_ptr);

    long long new_timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    float duration = (new_timestamp - m_timestamp) / 1e9f;
    m_box_rotation_angle += static_cast<float>(math::pi * duration * .5f);
    m_timestamp = new_timestamp;

    m_allocation =
        m_basic_rendering_services.constantDataStream().allocateAndUpdate(m_cb_data_mapping);

    /*m_cmd_list_ptr->setRootDescriptorTable(0, m_srv_table.gpu_pointer);
    m_cmd_list_ptr->setRootDescriptorTable(1, m_sampler_table.gpu_pointer);*/
    m_cmd_list_ptr->setRootConstantBufferView(static_cast<uint32_t>(dxcompilation::ShaderFunctionConstantBufferRootIds::scene_uniforms), m_allocation->virtualGpuAddress());

    m_cmd_list_ptr->drawIndexedInstanced(36, 1, 0, 0, 0);

    //cmd_list.close();

    return true;
}
