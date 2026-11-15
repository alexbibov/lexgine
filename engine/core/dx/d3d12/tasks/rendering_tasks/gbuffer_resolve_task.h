#ifndef LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_GBUFFER_RESOLVE_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_GBUFFER_RESOLVE_TASK_H

#include <memory>
#include <optional>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/concurrency/schedulable_task.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "engine/core/dx/d3d12/caches/lexgine_core_dx_d3d12_caches_fwd.h"
#include "engine/core/dx/dxcompilation/shader_function.h"
#include "engine/core/dx/d3d12/rendering/lexgine_core_dx_d3d12_rendering_fwd.h"

#include "lexgine_core_dx_d3d12_tasks_rendering_tasks_fwd.h"
#include "rendering_work.h"

namespace lexgine::core::dx::d3d12::tasks::rendering_tasks
{

class GBufferResolveTask final : public RenderingWork
{
public:
    static std::shared_ptr<GBufferResolveTask> create(
        Globals& globals,
        BasicRenderingServices& rendering_services
    )
    {
        return std::shared_ptr<GBufferResolveTask>{new GBufferResolveTask{ globals, rendering_services }};
    }

    void setGbuffer(rendering::Gbuffer* p_gbuffer)
    {
        m_gbuffer = p_gbuffer;
    }

public:  // Required by RenderinWork
    void updateRenderingConfiguration(
        RenderingConfigurationUpdateFlags update_flags,
        RenderingConfiguration const& rendering_configuration
    ) override;

private:    // required by AbstractTask interface
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override { return concurrency::TaskType::cpu; }

private:
    GBufferResolveTask(Globals& globals, BasicRenderingServices& rendering_services);

private:
    Globals& m_globals;
    Device& m_device;
    BasicRenderingServices& m_basic_rendering_services;
    rendering::Gbuffer* m_gbuffer{ nullptr };
    CommandList* m_cmd_list_ptr;
    dxcompilation::ShaderFunction m_shader_function;
    std::optional<dxcompilation::ShaderFunctionDescriptorTable> m_texture_table;
    std::optional<dxcompilation::ShaderFunctionDescriptorTable> m_sampler_table;
    caches::RootSignatureHandle m_rs_handle { nullptr };
    caches::GraphicsPSOHandle m_pso_handle { nullptr };
    PipelineState const* m_pso{ nullptr };
    CompiledRootSignature const* m_compiled_rs{ nullptr };
};

}

#endif
