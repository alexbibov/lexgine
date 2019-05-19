#include "lexgine/core/globals.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/viewport.h"
#include "lexgine/core/dx/d3d12/dx_resource_factory.h"
#include "lexgine/core/dx/d3d12/constant_buffer_stream.h"

#include "device.h"
#include "rendering_target.h"
#include "frame_progress_tracker.h"
#include "basic_rendering_services.h"

using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;

std::string const BasicRenderingServices::c_upload_section_name = "data_upload_section";
float const BasicRenderingServices::c_upload_section_fraction = .8f;

namespace {

size_t getUploadDataSectionOffset(Globals& globals)
{
    auto& global_settings = *globals.get<GlobalSettings>();
    size_t offset = static_cast<size_t>(std::ceilf(global_settings.getUploadHeapCapacity()
        * global_settings.getStreamedConstantDataPartitioning()));
    return misc::align(offset, 256);
}

}


BasicRenderingServices::BasicRenderingServices(Globals& globals)
    : m_device{ *globals.get<Device>() }
    , m_dx_resources{*globals.get<DxResourceFactory>()}
    , m_current_rendering_target_ptr{ nullptr }
    , m_rendering_target_color_format{ DXGI_FORMAT_UNKNOWN }
    , m_rendering_target_depth_format{ DXGI_FORMAT_UNKNOWN }
    , m_constant_data_stream{ globals }
{
    {    // initialize descriptor heap pages
        m_page0_descriptor_heaps.resize(2);
        for (size_t i = 0; i < 2; ++i)
        {
            DescriptorHeap const& descriptor_heap = m_dx_resources.retrieveDescriptorHeap(m_device, static_cast<DescriptorHeapType>(i), 0);
            m_page0_descriptor_heaps[i] = &descriptor_heap;
        }
    }

    {    // initialize resource upload stream allocator
        auto& global_settings = *globals.get<GlobalSettings>();
        size_t upload_stream_capacity = static_cast<size_t>(
            (global_settings.getUploadHeapCapacity() - getUploadDataSectionOffset(globals))
            * static_cast<double>(BasicRenderingServices::c_upload_section_fraction)
            );

        auto& dx_resource_factory = *globals.get<DxResourceFactory>();
        Device& device = *globals.get<Device>();
        auto upload_heap_section = dx_resource_factory.allocateSectionInUploadHeap(dx_resource_factory.retrieveUploadHeap(device),
            BasicRenderingServices::c_upload_section_name, upload_stream_capacity);

        if (!upload_heap_section.isValid())
        {
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
                "Unable to allocate section \"" + BasicRenderingServices::c_upload_section_name + "\" in upload heap");
        }

        UploadHeapPartition const& upload_heap_partition = static_cast<UploadHeapPartition const&>(upload_heap_section);
        m_resource_upload_allocator = std::make_unique<DedicatedUploadDataStreamAllocator>(globals, upload_heap_partition.offset, upload_heap_partition.size);
    }
}

void BasicRenderingServices::beginRendering(CommandList& command_list) const
{
    command_list.setDescriptorHeaps(m_page0_descriptor_heaps);
    m_current_rendering_target_ptr->switchToRenderAccessState(command_list);
}

void BasicRenderingServices::endRendering(CommandList& command_list) const
{
    m_current_rendering_target_ptr->switchToInitialState(command_list);
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
