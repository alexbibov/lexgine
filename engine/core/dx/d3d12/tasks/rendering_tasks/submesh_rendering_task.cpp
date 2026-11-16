#include <cstring>

#include "engine/core/globals.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/basic_rendering_services.h"
#include "engine/core/dx/d3d12/rendering_target.h"
#include "engine/core/dx/d3d12/rendering/gbuffer.h"

#include "engine/scenegraph/scene.h"
#include "engine/scenegraph/mesh.h"
#include "engine/scenegraph/submesh.h"
#include "engine/scenegraph/material.h"

#include "submesh_rendering_task.h"


namespace lexgine::core::dx::d3d12::tasks::rendering_tasks
{

SubmeshRenderingTask::SubmeshRenderingTask(Globals& globals, BasicRenderingServices& rendering_services)
    : RenderingWork{ globals, "Submesh rendering task", CommandType::direct }
    , m_device{ globals.device() }
    , m_basic_rendering_services{ rendering_services }
    , m_cmd_list_ptr{ addCommandList() }
{

}


void SubmeshRenderingTask::updateRenderingConfiguration(
    RenderingConfigurationUpdateFlags update_flags,
    RenderingConfiguration const& rendering_configuration
)
{

}


bool SubmeshRenderingTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    std::shared_ptr<scenegraph::Scene> source_scene = m_source_scene.lock();
    if (source_scene && !source_scene->loadStatus())
    {
        source_scene.reset();    // the scene is still loading; render the empty GBuffer this frame
    }

    if (!m_gbuffer)
    {
        return true;
    }

    RenderingTarget const& gbuffer_target = m_gbuffer->renderingTarget();
    gbuffer_target.switchToRenderAccessState(*m_cmd_list_ptr);
    m_cmd_list_ptr->outputMergerSetRenderTargets(&gbuffer_target.rtvTable(), (1ULL << gbuffer_target.count()) - 1,
        &gbuffer_target.dsvTable(), 0U);

    math::Vector4f const black_clear_color{ 0.f, 0.f, 0.f, 0.f };
    for (uint32_t i = 0; i < gbuffer_target.count(); ++i)
    {
        m_cmd_list_ptr->clearRenderTargetView(gbuffer_target.rtvTable(), i, black_clear_color);
    }
    m_cmd_list_ptr->clearDepthStencilView(gbuffer_target.dsvTable(), 0U, DSVClearFlags::clear_depth, 1.f, 0U);

    if (!source_scene)
    {
        gbuffer_target.switchToInitialState(*m_cmd_list_ptr);
        return true;
    }

    source_scene->updateGeometryTransforms();

    std::span<scenegraph::Scene::PerInstanceGpuData const> instances = source_scene->instancesGpuData();
    if (instances.empty())
    {
        gbuffer_target.switchToInitialState(*m_cmd_list_ptr);
        return true;
    }

    size_t const instance_data_size = instances.size() * sizeof(scenegraph::Scene::PerInstanceGpuData);
    auto instance_allocation = m_basic_rendering_services.dynamicGeometryStream().allocate(instance_data_size);
    if (!instance_allocation)
    {
        gbuffer_target.switchToInitialState(*m_cmd_list_ptr);
        return true;
    }
    std::memcpy(instance_allocation->cpuAddress(), instances.data(), instance_data_size);
    Resource const& upload_resource = m_basic_rendering_services.dynamicGeometryStream().getUploadResource();
    uint64_t const first_instance = instance_allocation->offset() / sizeof(scenegraph::Scene::PerInstanceGpuData);

    m_basic_rendering_services.setDefaultResources(*m_cmd_list_ptr);
    m_basic_rendering_services.setDefaultViewport(*m_cmd_list_ptr);
    m_cmd_list_ptr->inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle_list);

    std::span<scenegraph::Scene::Draw const> draws = source_scene->draws();
    for (scenegraph::Scene::DrawQuery const& draw_query : source_scene->drawQueries())
    {
        scenegraph::Material const& material = source_scene->getSceneMaterial(draw_query.material_id);
        scenegraph::MaterialStaticState const& static_state = material.getStaticState();
        if (!static_state.bindPipeline(*m_cmd_list_ptr))
        {
            continue;    // the pipeline is not compiled yet; retry on the next frame
        }
        static_state.bindDescriptorTables(*m_cmd_list_ptr);
        static_state.bindInstanceData(*m_cmd_list_ptr, upload_resource, first_instance);
        static_state.bindSceneParameters(*m_cmd_list_ptr, source_scene->getSceneConstants());
        static_state.bindMaterialParameters(*m_cmd_list_ptr, material.getMaterialConstants());

        for (uint32_t draw_id : draw_query.draw_ids)
        {
            scenegraph::Scene::Draw const& draw = draws[draw_id];
            scenegraph::Submesh const& submesh = source_scene->getSceneMesh(draw.draw_instance_id.mesh_id)
                .getSubmesh(draw.draw_instance_id.submesh_id);
            uint32_t const instance_count = static_cast<uint32_t>(draw.instance_indices.size());

            m_cmd_list_ptr->inputAssemblySetVertexBuffers(submesh.getVertexBufferBinding());

            D3D12_INDEX_BUFFER_VIEW const& ib_view = submesh.getIndexBufferBinding().indexBufferView();
            uint32_t const index_count = ib_view.SizeInBytes != 0
                ? static_cast<uint32_t>(ib_view.SizeInBytes / (ib_view.Format == DXGI_FORMAT_R32_UINT ? 4u : 2u))
                : 0u;
            if (index_count > 0)
            {
                m_cmd_list_ptr->inputAssemblySetIndexBuffer(submesh.getIndexBufferBinding());
                m_cmd_list_ptr->drawIndexedInstanced(index_count, instance_count, 0U, 0U, draw.gpu_instancing_section_start_index);
            }
            else
            {
                D3D12_VERTEX_BUFFER_VIEW const& vb_view = submesh.getVertexBufferBinding().vertexBufferViewAtSlot(0u);
                m_cmd_list_ptr->drawInstanced(vb_view.SizeInBytes / vb_view.StrideInBytes, instance_count, 0U, draw.gpu_instancing_section_start_index);
            }
        }
    }

    gbuffer_target.switchToInitialState(*m_cmd_list_ptr);
    return true;
}

}