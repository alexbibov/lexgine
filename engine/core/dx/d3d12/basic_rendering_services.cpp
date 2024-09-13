#include "engine/core/globals.h"
#include "engine/core/exception.h"
#include "engine/core/viewport.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/d3d12/constant_buffer_stream.h"
#include "engine/core/dx/dxcompilation/shader_function.h"
#include "engine/conversion/texture_converter.h"

#include "device.h"
#include "rendering_target.h"
#include "frame_progress_tracker.h"
#include "basic_rendering_services.h"


namespace lexgine::core::dx::d3d12
{

std::string const BasicRenderingServices::c_dynamic_geometry_section_name = "dynamic_geometry_section";

namespace {

    PerFrameUploadDataStreamAllocator createDynamicGeometryStreamAllocator(Globals& globals)
    {
        GlobalSettings& global_settings = *globals.get<GlobalSettings>();
        DxResourceFactory& dx_resource_factory = *globals.get<DxResourceFactory>();
        Device& device = *globals.get<Device>();
        Heap& upload_heap = dx_resource_factory.retrieveUploadHeap(device);

        UploadHeapPartition upload_heap_partition {};
        {
            auto upload_heap_partition_handle = dx_resource_factory.allocateSectionInUploadHeap(upload_heap, BasicRenderingServices::c_dynamic_geometry_section_name, global_settings.getStreamedGeometryDataPartitionSize());
            if (!upload_heap_partition_handle.isValid()) {
                LEXGINE_THROW_ERROR("Unable to allocate " + BasicRenderingServices::c_dynamic_geometry_section_name + " in upload heap");
            }
            upload_heap_partition = static_cast<UploadHeapPartition&>(upload_heap_partition_handle);
        }

        return PerFrameUploadDataStreamAllocator { globals, upload_heap_partition.offset, upload_heap_partition.size, device.frameProgressTracker() };
    }

}  // namespace


BasicRenderingServices::BasicRenderingServices(Globals& globals)
    : m_device{ *globals.get<Device>() }
    , m_dx_resources{ *globals.get<DxResourceFactory>() }
    , m_resource_uploader{ globals.get<conversion::TextureConverter>()->getDataUploader() }
    , m_static_cbv_srv_uav_descriptor_allocator{ m_dx_resources.getStaticAllocationManagerForDescriptorHeap(m_device, DescriptorHeapType::cbv_srv_uav, 0) }
    , m_static_sampler_descriptor_allocator{ m_dx_resources.getStaticAllocationManagerForDescriptorHeap(m_device, DescriptorHeapType::sampler, 0) }
    , m_current_rendering_target_ptr{ nullptr }
    , m_rendering_target_color_format{ DXGI_FORMAT_UNKNOWN }
    , m_rendering_target_depth_format{ DXGI_FORMAT_UNKNOWN }
    , m_constant_data_stream{ globals }
    , m_dynamic_geometry_allocator{ createDynamicGeometryStreamAllocator(globals) }
{
    {    // initialize descriptor heap pages
        m_page0_descriptor_heaps.resize(2);
        for (size_t i = 0; i < static_cast<size_t>(DescriptorHeapType::rtv); ++i)
        {
            DescriptorHeap const& descriptor_heap = m_dx_resources.retrieveDescriptorHeap(m_device, static_cast<DescriptorHeapType>(i), 0);
            m_page0_descriptor_heaps[i] = &descriptor_heap;
        }
    }
}

void BasicRenderingServices::beginRendering(CommandList& command_list) const
{
    m_current_rendering_target_ptr->switchToRenderAccessState(command_list);
}

void BasicRenderingServices::endRendering(CommandList& command_list) const
{
    m_current_rendering_target_ptr->switchToInitialState(command_list);
}

void BasicRenderingServices::setDefaultResources(CommandList& command_list) const
{
    command_list.setDescriptorHeaps(m_page0_descriptor_heaps);
    command_list.setRootDescriptorTable(static_cast<uint32_t>(dxcompilation::ShaderFunctionConstantBufferRootIds::count), m_static_cbv_srv_uav_descriptor_allocator.getBaseGpuAddress());
    command_list.setRootDescriptorTable(static_cast<uint32_t>(dxcompilation::ShaderFunctionConstantBufferRootIds::count) + 1, m_static_sampler_descriptor_allocator.getBaseGpuAddress());
}

void BasicRenderingServices::setDefaultViewport(CommandList& command_list) const
{
    command_list.rasterizerStateSetScissorRectangles(m_default_scissor_rectangles);
    command_list.rasterizerStateSetViewports(m_default_viewports);
}

void BasicRenderingServices::setDefaultRenderingTarget(CommandList& command_list) const
{
    RenderTargetViewDescriptorTable const& rtv_table = m_current_rendering_target_ptr->rtvTable();
    DepthStencilViewDescriptorTable const& dsv_table_ptr = m_current_rendering_target_ptr->dsvTable();

    command_list.outputMergerSetRenderTargets(&rtv_table, (1ULL << m_current_rendering_target_ptr->count()) - 1,
        m_current_rendering_target_ptr->hasDepth() ? &dsv_table_ptr : nullptr, 0U);
}

void BasicRenderingServices::clearDefaultRenderingTarget(CommandList& command_list,
    math::Vector4f const& color_clear_value, float depth_clear_value) const
{
    command_list.clearRenderTargetView(m_current_rendering_target_ptr->rtvTable(), 0, color_clear_value);
    command_list.clearDepthStencilView(m_current_rendering_target_ptr->dsvTable(), 0, DSVClearFlags::clear_depth, depth_clear_value, 0);
}

void BasicRenderingServices::defineRenderingViewport(Viewport const& viewport)
{
    m_default_viewports.clear();
    m_default_viewports.push_back(viewport);

    m_default_scissor_rectangles.clear();
    m_default_scissor_rectangles.push_back(math::Rectangle{ math::Vector2f{0.f, 0.f}, viewport.width(), viewport.height() });
}

} // namespace lexgine::core::dx::d3d12