#include <algorithm>
#include <limits>

#include "engine/core/debug_textures_registry.h"

#include "engine/core/global_settings.h"
#include "engine/core/multisampling.h"
#include "engine/core/dx/d3d12/device.h"

#include "gbuffer.h"

namespace lexgine::core::dx::d3d12::rendering
{

Gbuffer::Gbuffer(Globals& globals)
    : m_globals{ globals }
    , m_globals_settings{ globals.d3d12().globalSettings() }
    , m_device{ m_globals.d3d12().device() }
{

}

void Gbuffer::init(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    DXGI_FORMAT const gbuffer_formats[] = {
        DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_D32_FLOAT
    };
    MultiSamplingFormat const msaa_format = queryMultisamplingFormat(gbuffer_formats);

    std::vector<ResourceDescriptor> descriptors{
        ResourceDescriptor::createTexture2D(
            m_width,
            m_height,
            1,
            DXGI_FORMAT_R11G11B10_FLOAT,
            1,
            ResourceFlags::base_values::render_target,
            msaa_format
        ),
        ResourceDescriptor::createTexture2D(
            m_width,
            m_height,
            1,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            1,
            ResourceFlags::base_values::render_target,
            msaa_format
        ),
        ResourceDescriptor::createTexture2D(
            m_width,
            m_height,
            1,
            DXGI_FORMAT_R16G16_FLOAT,
            1,
            ResourceFlags::base_values::render_target,
            msaa_format
        ),
        ResourceDescriptor::createTexture2D(
            m_width,
            m_height,
            1,
            DXGI_FORMAT_D32_FLOAT,
            1,
            ResourceFlags::base_values::depth_stencil,
            msaa_format
        )
    };

    bool const is_msaa_enabled = msaa_format.count > 1;
    std::vector<ResourceHeapAllocationInfo> allocation_info;
    ResourceSetHeapAllocationInfo set_allocation_info = m_device.queryResourceAllocationInfo(descriptors, &allocation_info);

    Heap heap = m_device.createHeap(AbstractHeapType::_default, set_allocation_info.size_in_bytes,
        HeapCreationFlags::base_values::allow_only_rt_ds, is_msaa_enabled);
    m_heap = std::make_unique<Heap>(std::move(heap));

    math::Vector4f const black_clear_color{ 0.f, 0.f, 0.f, 0.f };

    m_albedo = PlacedResource(*m_heap, allocation_info[0].offset,
        ResourceState::base_values::pixel_shader,
        ResourceOptimizedClearValue{ DXGI_FORMAT_R11G11B10_FLOAT, black_clear_color }, descriptors[0]);
    m_albedo.setStringName("gbuffer_albedo");
    m_globals.debugTextureRegistry().registerDebugTexture(m_albedo);
    

    m_nmre = PlacedResource(*m_heap, allocation_info[1].offset,
        ResourceState::base_values::pixel_shader,
        ResourceOptimizedClearValue{ DXGI_FORMAT_R16G16B16A16_FLOAT, black_clear_color }, descriptors[1]);
    m_nmre.setStringName("gbuffer_nmre");
    m_globals.debugTextureRegistry().registerDebugTexture(m_nmre);

    m_emission = PlacedResource(*m_heap, allocation_info[2].offset,
        ResourceState::base_values::pixel_shader,
        ResourceOptimizedClearValue{ DXGI_FORMAT_R16G16_FLOAT, black_clear_color }, descriptors[2]);
    m_emission.setStringName("gbuffer_emission");
    m_globals.debugTextureRegistry().registerDebugTexture(m_emission);

    m_depth = PlacedResource(*m_heap, allocation_info[3].offset,
        ResourceState::base_values::depth_read,
        ResourceOptimizedClearValue{ DXGI_FORMAT_D32_FLOAT, DepthStencilValue{ 1.f, 0 } }, descriptors[3]);
    m_depth.setStringName("gbuffer_depth");
    m_globals.debugTextureRegistry().registerDebugTexture(m_depth);

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

        m_resolved_albedo = PlacedResource(*m_resolved_heap, resolve_resources_allocation_info[0].offset,
            ResourceState::base_values::pixel_shader,
            misc::Optional<ResourceOptimizedClearValue>{}, resolve_descriptors[0]);
        m_resolved_albedo.setStringName("gbuffer_resolved_albedo");
    }
    else
    {
        m_resolved_albedo = {};
    }

    std::vector<ColorTarget> color_targets{
        ColorTarget{ m_albedo, ResourceState::base_values::pixel_shader, RTVTextureInfo{} },
        ColorTarget{ m_nmre, ResourceState::base_values::pixel_shader, RTVTextureInfo{} },
        ColorTarget{ m_emission, ResourceState::base_values::pixel_shader, RTVTextureInfo{} }
    };
    DepthTarget depth_target{ m_depth, ResourceState::base_values::depth_read, DSVTextureInfo{} };
    m_rendering_target = std::make_unique<RenderingTarget>(m_globals, color_targets, misc::Optional<DepthTarget>{ depth_target });
}

MultiSamplingFormat Gbuffer::queryMultisamplingFormat(std::span<DXGI_FORMAT const> dxgi_formats) const
{
    auto const sample_count = static_cast<uint32_t>(m_globals_settings.msaaMode());
    uint32_t num_quality_levels = (std::numeric_limits<uint32_t>::max)();
    for (DXGI_FORMAT format : dxgi_formats)
    {
        num_quality_levels = (std::min)(num_quality_levels, m_device.queryFeatureQualityLevels(format, sample_count).num_quality_levels);
    }

    if (num_quality_levels == 0 || num_quality_levels == (std::numeric_limits<uint32_t>::max)())
    {
        return MultiSamplingFormat{ 1, 0 };
    }
    return MultiSamplingFormat{ sample_count, num_quality_levels - 1 };
}

}