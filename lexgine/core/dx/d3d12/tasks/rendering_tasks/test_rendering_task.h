#ifndef LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_TEST_RENDERING_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_TEST_RENDERING_TASK_H

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "lexgine/core/concurrency/schedulable_task.h"

#include "lexgine/core/dx/d3d12/resource_data_uploader.h"
#include "lexgine/core/dx/d3d12/vertex_buffer.h"
#include "lexgine/core/dx/d3d12/constant_buffer_data_mapper.h"
#include "lexgine/core/dx/d3d12/descriptor_table_builders.h"


namespace lexgine::core::dx::d3d12::tasks::rendering_tasks {

class TestRenderingTask final : public concurrency::RootSchedulableTask
{
public:
    TestRenderingTask(Globals& globals, BasicRenderingServices& rendering_services);

    void updateBufferFormats(DXGI_FORMAT color_target_format, DXGI_FORMAT depth_target_format);
    
private:    // required by AbstractTask interface
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override { return concurrency::TaskType::gpu_draw; }

private:
    Globals& m_globals;
    Device& m_device;
    BasicRenderingServices& m_basic_rendering_services;

    ResourceDataUploader m_data_uploader;
    VertexBuffer m_vb;
    IndexBuffer m_ib;
    VertexAttributeSpecificationList m_va_list;

    CommittedResource m_texture;
    ConstantBufferReflection m_cb_reflection;
    ConstantBufferDataMapper m_cb_data_mapping;
    long long m_timestamp;
    float m_box_rotation_angle;
    math::Matrix4f m_projection_transform;

    CommandList m_cmd_list;

    RootSignatureCompilationTask* m_rs = nullptr;
    HLSLCompilationTask* m_vs = nullptr;
    HLSLCompilationTask* m_ps = nullptr;
    GraphicsPSOCompilationTask* m_pso = nullptr;

    std::array<float, 64> m_box_vertices;
    std::array<short, 36> m_box_indices;

    ShaderResourceDescriptorTable m_srv_table;
    ShaderResourceDescriptorTable m_sampler_table;

    core::Allocator<UploadDataBlock>::address_type m_allocation;
};

}

#endif
