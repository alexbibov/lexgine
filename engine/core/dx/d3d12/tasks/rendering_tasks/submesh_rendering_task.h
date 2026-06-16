#ifndef LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_SUBMESH_RENDERING_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_SUBMESH_RENDERING_TASK_H

#include <memory>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/concurrency/schedulable_task.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"
#include "engine/core/dx/d3d12/caches/lexgine_core_dx_d3d12_caches_fwd.h"

#include "engine/scenegraph/lexgine_scenegraph_fwd.h"

#include "lexgine_core_dx_d3d12_tasks_rendering_tasks_fwd.h"
#include "rendering_work.h"

namespace lexgine::core::dx::d3d12::tasks::rendering_tasks
{

class SubmeshRenderingTask final : public RenderingWork
{
public:
    static std::shared_ptr<SubmeshRenderingTask> create(
        Globals& globals, 
        BasicRenderingServices& rendering_services
    )
    {
        return std::shared_ptr<SubmeshRenderingTask>{new SubmeshRenderingTask{ globals, rendering_services }};
    }

    void setSourceScene(std::shared_ptr<scenegraph::Scene> const& source_scene);

public:  // Required by RenderinWork
    void updateRenderingConfiguration(
        RenderingConfigurationUpdateFlags update_flags,
        RenderingConfiguration const& rendering_configuration
    ) override;

private:    // required by AbstractTask interface
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override { return concurrency::TaskType::cpu; }

private:
    SubmeshRenderingTask(Globals& globals, BasicRenderingServices& rendering_services);

private:
    Device& m_device;
    BasicRenderingServices& m_basic_rendering_services;
    std::weak_ptr<scenegraph::Scene> m_source_scene;
};

}

#endif