#include "engine/core/global_settings.h"
#include "engine/core/multisampling.h"
#include "engine/core/dx/d3d12/device.h"

#include "gbuffer.h"

namespace lexgine::core::dx::d3d12::rendering
{

Gbuffer::Gbuffer(Globals& globals)
    : m_globals{ globals }
    , m_globals_settings{ globals.globalSettings() }
    , m_device{ m_globals.device() }
{

}

void Gbuffer::init(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    std::vector<ResourceDescriptor> descriptors{
        ResourceDescriptor::createTexture2D(
            m_width,
            m_height,
            1,
            DXGI_FORMAT_R11G11B10_FLOAT,
            1,
            ResourceFlags::base_values::render_target,
            queryMultisamplingFormatForDxgiSurface(DXGI_FORMAT_R11G11B10_FLOAT)
        ),
        ResourceDescriptor::createTexture2D(
            m_width,
            m_height,
            1,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            1,
            ResourceFlags::base_values::render_target,
            queryMultisamplingFormatForDxgiSurface(DXGI_FORMAT_R16G16B16A16_FLOAT)
        ),
        ResourceDescriptor::createTexture2D(
            m_width,
            m_height,
            1,
            DXGI_FORMAT_R16G16_FLOAT,
            1,
            ResourceFlags::base_values::render_target,
            queryMultisamplingFormatForDxgiSurface(DXGI_FORMAT_R16G16_FLOAT)
        ),
        ResourceDescriptor::createTexture2D(
            m_width,
            m_height,
            1,
            DXGI_FORMAT_D32_FLOAT,
            1,
            ResourceFlags::base_values::depth_stencil,
            queryMultisamplingFormatForDxgiSurface(DXGI_FORMAT_D32_FLOAT)
        )
    };

    bool const is_msaa_enabled = m_globals_settings.msaaMode() != MSAAMode::none;
    std::vector<ResourceHeapAllocationInfo> allocation_info;
    ResourceSetHeapAllocationInfo set_allocation_info = m_device.queryResourceAllocationInfo(descriptors, &allocation_info);

    Heap heap = m_device.createHeap(AbstractHeapType::_default, set_allocation_info.size_in_bytes,
        HeapCreationFlags::base_values::allow_only_rt_ds, is_msaa_enabled);
    m_heap = std::make_unique<Heap>(std::move(heap));

    math::Vector4f const black_clear_color{ 0.f, 0.f, 0.f, 0.f };

    m_albedo = std::make_unique<PlacedResource>(*m_heap, allocation_info[0].offset,
        ResourceState::base_values::render_target,
        ResourceOptimizedClearValue{ DXGI_FORMAT_R11G11B10_FLOAT, black_clear_color }, descriptors[0]);
    m_albedo->setStringName("gbuffer_albedo");

    m_nmre = std::make_unique<PlacedResource>(*m_heap, allocation_info[1].offset,
        ResourceState::base_values::render_target,
        ResourceOptimizedClearValue{ DXGI_FORMAT_R16G16B16A16_FLOAT, black_clear_color }, descriptors[1]);
    m_nmre->setStringName("gbuffer_nmre");

    m_emission = std::make_unique<PlacedResource>(*m_heap, allocation_info[2].offset,
        ResourceState::base_values::render_target,
        ResourceOptimizedClearValue{ DXGI_FORMAT_R16G16_FLOAT, black_clear_color }, descriptors[2]);
    m_emission->setStringName("gbuffer_emission");

    m_depth = std::make_unique<PlacedResource>(*m_heap, allocation_info[3].offset,
        ResourceState::base_values::depth_write,
        ResourceOptimizedClearValue{ DXGI_FORMAT_D32_FLOAT, DepthStencilValue{ 1.f, 0 } }, descriptors[3]);
    m_depth->setStringName("gbuffer_depth");

    if (is_msaa_enabled)
    {
        std::vector<ResourceDescriptor> resolve_descriptors{};

        // the albedo target is multisampled and cannot be sampled directly; allocate a plain surface to resolve it into
        resolve_descriptors.push_back(ResourceDescriptor::createTexture2D(
            m_width,
            m_height,
            1,
            DXGI_FORMAT_R11G11B10_FLOAT,
            1,
            ResourceFlags::base_values::none,
            MultiSamplingFormat{}
        ));

        std::vector<ResourceHeapAllocationInfo> resolve_resources_allocation_info;
        ResourceSetHeapAllocationInfo resolve_resource_set_allocation_info = m_device.queryResourceAllocationInfo(resolve_descriptors, &resolve_resources_allocation_info);

        Heap resolve_heap = m_device.createHeap(
            AbstractHeapType::_default,
            resolve_resource_set_allocation_info.size_in_bytes,
            HeapCreationFlags::base_values::allow_only_non_rt_ds_textures
        );
        m_resolved_heap = std::make_unique<Heap>(std::move(resolve_heap));

        m_resolved_albedo = std::make_unique<PlacedResource>(*m_resolved_heap, resolve_resources_allocation_info[0].offset,
            ResourceState::base_values::pixel_shader,
            misc::Optional<ResourceOptimizedClearValue>{}, resolve_descriptors[0]);
        m_resolved_albedo->setStringName("gbuffer_resolved_albedo");
    }
    else
    {
        m_resolved_albedo.reset();
    }

    std::vector<ColorTarget> color_targets{
        ColorTarget{ *m_albedo, ResourceState::base_values::pixel_shader, RTVTextureInfo{} },
        ColorTarget{ *m_nmre, ResourceState::base_values::pixel_shader, RTVTextureInfo{} },
        ColorTarget{ *m_emission, ResourceState::base_values::pixel_shader, RTVTextureInfo{} }
    };
    DepthTarget depth_target{ *m_depth, ResourceState::base_values::depth_write, DSVTextureInfo{} };
    m_rendering_target = std::make_unique<RenderingTarget>(m_globals, color_targets, misc::Optional<DepthTarget>{ depth_target });
}

MultiSamplingFormat Gbuffer::queryMultisamplingFormatForDxgiSurface(DXGI_FORMAT dxgi_format) const
{
    MSAAMode msaa_mode = m_globals_settings.msaaMode();
    FeatureMultisampleQualityLevels msaa_quality = m_device.queryFeatureQualityLevels(dxgi_format, static_cast<uint32_t>(msaa_mode));
    MultiSamplingFormat fmt{ static_cast<uint32_t>(msaa_mode), msaa_quality.num_quality_levels > 0 ? msaa_quality.num_quality_levels - 1 : 0 };
    return fmt;
}

}