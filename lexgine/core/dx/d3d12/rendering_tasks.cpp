#include "rendering_tasks.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/logging_streams.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/math/vector_types.h"

#include "lexgine/core/dx/d3d12/tasks/root_signature_compilation_task.h"
#include "lexgine/core/dx/d3d12/tasks/hlsl_compilation_task.h"
#include "lexgine/core/dx/d3d12/tasks/pso_compilation_task.h"

#include "dx_resource_factory.h"
#include "device.h"
#include "command_list.h"
#include "rendering_target.h"
#include "frame_progress_tracker.h"

#include "resource_data_uploader.h"
#include "vertex_buffer.h"


using namespace lexgine::core;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks;
using namespace lexgine::core::dx::d3d12::task_caches;


namespace {

std::vector<std::ostream*> convertFileStreamsToGenericStreams(std::vector<std::ofstream>& fstreams)
{
    std::vector<std::ostream*> res(fstreams.size());
    std::transform(fstreams.begin(), fstreams.end(), res.begin(), [](std::ofstream& fs)->std::ostream* { return &fs; });
    return res;
}

}


class RenderingTasks::FrameBeginTask final : public RootSchedulableTask
{
public:
    FrameBeginTask(RenderingTasks& rendering_tasks,
        math::Vector4f const& clear_color) :
        RootSchedulableTask{ "frame_begin_task" },
        m_rendering_tasks{ rendering_tasks },
        m_clear_color{ clear_color },
        m_command_list{ rendering_tasks.m_device.createCommandList(CommandType::direct, 0x1) }
    {
        m_command_list.setStringName("clear_screen");

        m_page0_descriptor_heaps.resize(2U);    // we only fetch two heaps: cbv_srv_uav and sampler since only these two must be set from the command list
        for (int i = 0; i < 2U; ++i)
        {
            m_page0_descriptor_heaps[i] =
                &m_rendering_tasks.m_dx_resources.retrieveDescriptorHeap(rendering_tasks.m_device, static_cast<DescriptorHeapType>(i), 0);
        }
    }

    void setClearColor(math::Vector4f const& color)
    {
        m_clear_color = color;
    }

private:    // required by the AbstractTask interface
    bool doTask(uint8_t worker_id, uint64_t current_frame_index) override
    {
        auto& target = *m_rendering_tasks.m_current_rendering_target_ptr;

        m_command_list.reset();
        m_command_list.setDescriptorHeaps(m_page0_descriptor_heaps);
        //m_command_list.inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle);

        /*m_command_list.outputMergerSetRenderTargets(&target.rtvTable(), 1,
            target.hasDepth() ? &target.dsvTable() : nullptr, 0U);

        target.switchToRenderAccessState(m_command_list);

        m_command_list.clearRenderTargetView(target.rtvTable(), 0, m_clear_color);*/

        m_command_list.close();

        FrameProgressTrackerAttorney<RenderingTasks>::signalGPUBeginFrame(m_rendering_tasks.m_frame_progress_tracker,
            m_rendering_tasks.m_device.defaultCommandQueue());
        m_rendering_tasks.m_device.defaultCommandQueue().executeCommandList(m_command_list);

        return true;
    }

    TaskType type() const override
    {
        return TaskType::gpu_draw;
    }

private:
    RenderingTasks& m_rendering_tasks;
    misc::StaticVector<DescriptorHeap const*, 4U> m_page0_descriptor_heaps;
    math::Vector4f m_clear_color;
    CommandList m_command_list;
};

class RenderingTasks::FrameEndTask final : public SchedulableTask
{
    public:
    FrameEndTask(RenderingTasks& rendering_tasks) :
        SchedulableTask{ "frame_end_task" },
        m_rendering_tasks{ rendering_tasks },
        m_command_list{ m_rendering_tasks.m_device.createCommandList(CommandType::direct, 0x1) }
    {

    }

private:    // required by the AbstractTask interface
    bool doTask(uint8_t worker_id, uint64_t current_frame_index) override
    {

        auto& target = *m_rendering_tasks.m_current_rendering_target_ptr;

        m_command_list.reset();

        // target.switchToInitialState(m_command_list);

        m_command_list.close();
        m_rendering_tasks.m_device.defaultCommandQueue().executeCommandList(m_command_list);
        FrameProgressTrackerAttorney<RenderingTasks>::signalGPUEndFrame(m_rendering_tasks.m_frame_progress_tracker,
            m_rendering_tasks.m_device.defaultCommandQueue());

        return true;
    }

    TaskType type() const override
    {
        return TaskType::gpu_draw;
    }

private:
    RenderingTasks& m_rendering_tasks;
    CommandList m_command_list;
};


class RenderingTasks::TestTriangleRendering final
{
public:
    TestTriangleRendering(Globals& globals)
        : m_device{ *globals.get<Device>() }
        , m_data_uploader{ globals, 0, 64 * 1024 * 1024 }
        , m_vb{ m_device }
        , m_ib{ m_device, IndexDataType::_16_bit, 32 * 1024 }
        , m_texture{ m_device, ResourceState::enum_type::pixel_shader, misc::makeEmptyOptional<ResourceOptimizedClearValue>(),
                    ResourceDescriptor::CreateTexture2D(256, 256, 1, DXGI_FORMAT_R8G8B8A8_UNORM), AbstractHeapType::default,
                    HeapCreationFlags::enum_type::allow_all }
    {
        std::shared_ptr<AbstractVertexAttributeSpecification> position = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
            std::make_shared<VertexAttributeSpecification<float, 3>>(0, 24, "SV_Position", 0, 0));
        std::shared_ptr<AbstractVertexAttributeSpecification> color = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
            std::make_shared<VertexAttributeSpecification<float, 3>>(0, 24, "COLOR", 0, 0));

        VertexAttributeSpecificationList va_spec_list{ position, color };


        {
            m_vb.setSegment(va_spec_list, 8, 0);
            m_vb.build();

            ResourceDataUploader::DestinationDescriptor destination_descriptor;
            destination_descriptor.p_destination_resource = &m_vb.resource();
            destination_descriptor.destination_resource_state = ResourceState::enum_type::vertex_and_constant_buffer;
            destination_descriptor.segment.base_offset = 0U;

            m_box_vertices = std::array<float, 48>{
                -1.f, -1.f, -1.f, 1.f, 0.f, 0.f,
                 1.f, -1.f, -1.f, 0.f, 1.f, 0.f,
                 1.f, 1.f, -1.f, 0.f, 0.f, 1.f,
                -1.f, 1.f, -1.f, .5f, .5f, .5f,

                -1.f, -1.f, 1.f, .5f, .5f, .5f,
                 1.f, -1.f, 1.f, 0.f, 0.f, 1.f,
                 1.f, 1.f, 1.f, 0.f, 1.f, 0.f,
                -1.f, 1.f, 1.f, 1.f, 0.f, 0.f,
            };

            ResourceDataUploader::BufferSourceDescriptor source_descriptor;
            source_descriptor.p_data = m_box_vertices.data();
            source_descriptor.buffer_size = m_box_vertices.size() * sizeof(float);

            m_data_uploader.addResourceForUpload(destination_descriptor, source_descriptor);
        }


        {
            ResourceDataUploader::DestinationDescriptor destination_descriptor;
            destination_descriptor.p_destination_resource = &m_ib.resource();
            destination_descriptor.destination_resource_state = ResourceState::enum_type::index_buffer;
            destination_descriptor.segment.base_offset = 0U;

            m_box_indices = std::array<short, 36>{
                0, 3, 1, 1, 3, 2,
                4, 5, 7, 5, 6, 7,
                7, 6, 3, 6, 2, 3,
                4, 0, 5, 5, 0, 1,
                5, 1, 6, 1, 2, 6,
                0, 4, 7, 0, 7, 3
            };

            ResourceDataUploader::BufferSourceDescriptor source_descriptor;
            source_descriptor.p_data = m_box_indices.data();
            source_descriptor.buffer_size = m_box_indices.size() * sizeof(short);

            m_data_uploader.addResourceForUpload(destination_descriptor, source_descriptor);
        }

        m_data_uploader.upload();
        m_data_uploader.waitUntilUploadIsFinished();


        RootSignatureCompilationTask* rs_compilation_task{ nullptr };
        {
            // Create root signature
            RootSignatureCompilationTaskCache& rs_compilation_task_cache = *globals.get<RootSignatureCompilationTaskCache>();
            RootSignature rs{};
            
            RootEntryDescriptorTable main_parameters;
            main_parameters.addRange(RootEntryDescriptorTable::RangeType::srv, 1, 0, 0, 0);
            main_parameters.addRange(RootEntryDescriptorTable::RangeType::cbv, 1, 0, 0, 1);

            RootEntryDescriptorTable sampler_table;
            sampler_table.addRange(RootEntryDescriptorTable::RangeType::sampler, 1, 0, 0, 0);

            rs.addParameter(0, main_parameters);
            rs.addParameter(1, sampler_table);

            
            rs_compilation_task = rs_compilation_task_cache.addTask(globals, std::move(rs), RootSignatureFlags::enum_type::none,
                "test_rendering_rs", 0);
            rs_compilation_task->execute(0);
        }
    }

private:
    Device const& m_device;
    ResourceDataUploader m_data_uploader;
    VertexBuffer m_vb;
    IndexBuffer m_ib;
    CommittedResource m_texture;

    std::array<float, 48> m_box_vertices;
    std::array<short, 36> m_box_indices;
};


RenderingTasks::RenderingTasks(Globals& globals)
    : m_dx_resources{ *globals.get<DxResourceFactory>() }
    , m_device{ *globals.get<Device>() }
    , m_frame_progress_tracker{ m_dx_resources.retrieveFrameProgressTracker(m_device) }

    , m_task_graph{ globals.get<GlobalSettings>()->getNumberOfWorkers(), "RenderingTasksGraph" }
    , m_task_sink{ m_task_graph, convertFileStreamsToGenericStreams(globals.get<LoggingStreams>()->worker_logging_streams), "RenderingTasksSink" }

    , m_frame_begin_task{ new FrameBeginTask{*this, math::Vector4f{0.f, 0.f, 0.f, 0.f}} }
    , m_frame_end_task{ new FrameEndTask{*this} }

    // , m_test_triangle_rendering{ new TestTriangleRendering{ globals } }
{
    m_task_graph.setRootNodes({ m_frame_begin_task.get() });
    m_frame_begin_task->addDependent(*m_frame_end_task);

    m_task_sink.start();
}

RenderingTasks::~RenderingTasks()
{
    m_task_sink.shutdown();
    m_frame_progress_tracker.waitForFrameCompletion(m_frame_progress_tracker.lastScheduledFrameIndex());
}


void RenderingTasks::render(RenderingTarget& target,
    std::function<void(RenderingTarget const&)> const& presentation_routine)
{
    FrameProgressTrackerAttorney<RenderingTasks>::signalCPUBeginFrame(m_frame_progress_tracker);
    m_current_rendering_target_ptr = &target;
    m_task_sink.submit(m_frame_progress_tracker.currentFrameIndex());
    presentation_routine(target);
    FrameProgressTrackerAttorney<RenderingTasks>::signalCPUEndFrame(m_frame_progress_tracker);
}


FrameProgressTracker const& RenderingTasks::frameProgressTracker() const
{
    return m_frame_progress_tracker;
}

