#include "engine/core/concurrency/thread_affinity.h"
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

namespace {

PerFrameUploadDataStreamAllocator createDynamicGeometryStreamAllocator(Globals& globals)
{
    GlobalSettings& global_settings = globals.globalSettings();
    DxResourceFactory& dx_resource_factory = globals.dxResourceFactory();
    Device& device = globals.device();
    Heap& upload_heap = dx_resource_factory.retrieveUploadHeap(device);

    UploadHeapPartition upload_heap_partition {};
    {
        auto upload_heap_partition_handle = dx_resource_factory.allocateSectionInUploadHeap(upload_heap, DxResourceFactory::c_dynamic_geometry_section_name, global_settings.getStreamedGeometryDataPartitionSize());
        if (!upload_heap_partition_handle.isValid()) {
            LEXGINE_THROW_ERROR("Unable to allocate " + std::string{ DxResourceFactory::c_dynamic_geometry_section_name } + " in upload heap");
        }
        upload_heap_partition = *upload_heap_partition_handle;
    }

    return PerFrameUploadDataStreamAllocator { globals, upload_heap_partition.offset, upload_heap_partition.size, device.frameProgressTracker() };
}

}  // namespace


BasicRenderingServices::BasicRenderingServices(Globals& globals)
    : ProvidesGlobals{ globals }
    , m_device{ globals.device() }
    , m_dx_resources{ globals.dxResourceFactory() }
    , m_resource_uploader{ globals.textureConverter().getDataUploader() }
    // one slot beyond the frames in flight, so that the context of the next frame may be prepared before the
    // gate admits it
    , m_frame_contexts(globals.globalSettings().getMaxFramesInFlight() + 1u, core::rendering::FrameContext{})
    , m_rendering_target_color_format{ DXGI_FORMAT_UNKNOWN }
    , m_rendering_target_depth_format{ DXGI_FORMAT_UNKNOWN }
    , m_constant_data_stream{ globals }
    , m_dynamic_geometry_allocator{ createDynamicGeometryStreamAllocator(globals) }
    , m_descriptor_heaps{
        &m_device.descriptorHeap(DescriptorHeapType::cbv_srv_uav),
        &m_device.descriptorHeap(DescriptorHeapType::sampler) }
{
}

core::rendering::FrameContext const& BasicRenderingServices::frameContext(uint64_t frame_index) const
{
    return m_frame_contexts[frame_index % m_frame_contexts.size()];
}

void BasicRenderingServices::defineFrameContext(uint64_t frame_index, RenderingTarget& rendering_target)
{
    concurrency::assertOnMasterWorkerThread();

    m_frame_contexts[frame_index % m_frame_contexts.size()] =
        core::rendering::FrameContext{ .frame_index = frame_index, .p_rendering_target = &rendering_target };
}

void BasicRenderingServices::beginRendering(CommandList& command_list, core::rendering::FrameContext const& frame_context) const
{
    concurrency::assertOnRenderThread();

    frame_context.p_rendering_target->switchToRenderAccessState(command_list);
}

void BasicRenderingServices::endRendering(CommandList& command_list, core::rendering::FrameContext const& frame_context) const
{
    concurrency::assertOnRenderThread();

    frame_context.p_rendering_target->switchToInitialState(command_list);
}

void BasicRenderingServices::setDefaultResources(CommandList& command_list) const
{
    concurrency::assertOnRenderThread();

    command_list.setDescriptorHeaps(std::span{ m_descriptor_heaps });
}

void BasicRenderingServices::setDefaultViewport(CommandList& command_list) const
{
    concurrency::assertOnRenderThread();

    command_list.rasterizerStateSetScissorRectangles(m_default_scissor_rectangles);
    command_list.rasterizerStateSetViewports(m_default_viewports);
}

void BasicRenderingServices::setDefaultRenderingTarget(CommandList& command_list,
    core::rendering::FrameContext const& frame_context) const
{
    concurrency::assertOnRenderThread();

    RenderingTarget& rendering_target = *frame_context.p_rendering_target;
    DescriptorTable const& rtv_table = rendering_target.rtvTable();
    DescriptorTable const& dsv_table_ptr = rendering_target.dsvTable();

    command_list.outputMergerSetRenderTargets(&rtv_table, (1ULL << rendering_target.count()) - 1,
        rendering_target.hasDepth() ? &dsv_table_ptr : nullptr, 0U);
}

void BasicRenderingServices::clearDefaultRenderingTarget(CommandList& command_list,
    core::rendering::FrameContext const& frame_context,
    math::Vector4f const& color_clear_value, float depth_clear_value) const
{
    concurrency::assertOnRenderThread();

    RenderingTarget& rendering_target = *frame_context.p_rendering_target;
    command_list.clearRenderTargetView(rendering_target.rtvTable(), 0, color_clear_value);
    command_list.clearDepthStencilView(rendering_target.dsvTable(), 0, DSVClearFlags::clear_depth, depth_clear_value, 0);
}

void BasicRenderingServices::defineRenderingViewport(Viewport const& viewport)
{
    m_default_viewports.clear();
    m_default_viewports.push_back(viewport);

    m_default_scissor_rectangles.clear();
    m_default_scissor_rectangles.push_back(math::Rectangle{ math::Vector2f{0.f, 0.f}, viewport.width(), viewport.height() });
}

} // namespace lexgine::core::dx::d3d12