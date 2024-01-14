#ifndef LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_TEST_RENDERING_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_TEST_RENDERING_TASK_H

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "engine/core/concurrency/schedulable_task.h"

#include "engine/core/dx/d3d12/resource_data_uploader.h"
#include "engine/core/dx/d3d12/vertex_buffer.h"
#include "engine/core/dx/d3d12/constant_buffer_data_mapper.h"
#include "engine/core/dx/d3d12/descriptor_table_builders.h"

#include "rendering_work.h"

namespace lexgine::core::dx::d3d12::tasks::rendering_tasks {

class TestRenderingTask final : public RenderingWork
{
public:
    static std::shared_ptr<TestRenderingTask> create(Globals& globals, BasicRenderingServices& rendering_services)
    {
        return std::shared_ptr<TestRenderingTask>{ new TestRenderingTask{ globals, rendering_services } };
    }

    void updateRenderingConfiguration(RenderingConfigurationUpdateFlags update_flags, RenderingConfiguration const& rendering_configuration) override;

private:    // required by AbstractTask interface
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override { return concurrency::TaskType::cpu; }

private:
    TestRenderingTask(Globals& globals, BasicRenderingServices& rendering_services);

private:
    Globals& m_globals;
    Device& m_device;
    BasicRenderingServices& m_basic_rendering_services;

    VertexBuffer m_vb;
    IndexBuffer m_ib;
    VertexAttributeSpecificationList m_va_list;

    CommittedResource m_texture;
    ConstantBufferReflection m_cb_reflection;
    ConstantBufferDataMapper m_cb_data_mapping;
    long long m_timestamp;
    float m_box_rotation_angle;
    math::Matrix4f m_projection_transform;

    RootSignatureCompilationTask* m_rs = nullptr;
    HLSLCompilationTask* m_vs = nullptr;
    HLSLCompilationTask* m_ps = nullptr;
    GraphicsPSOCompilationTask* m_pso = nullptr;

    std::array<float, 64> m_box_vertices;
    std::array<short, 36> m_box_indices;

    ShaderResourceDescriptorTable m_srv_table;
    ShaderResourceDescriptorTable m_sampler_table;

    core::Allocator<UploadDataBlock>::address_type m_allocation;

    CommandList* m_cmd_list_ptr = nullptr;
};

}

#endif
