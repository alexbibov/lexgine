#include "rendering_tasks.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/exception.h"
#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/core/math/vector_types.h"

#include "dx_resource_factory.h"
#include "device.h"
#include "command_list.h"
#include "rendering_target.h"
#include "profiler.h"


using namespace lexgine::core;
using namespace lexgine::core::concurrency;
using namespace lexgine::core::dx::d3d12;


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

        m_page0_descriptor_heaps.resize(4U);
        for (int i = 0; i < 4U; ++i)
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
    bool do_task(uint8_t worker_id, uint16_t frame_index) override
    {
        uint64_t current_frame_index = m_rendering_tasks.m_end_of_frame_cpu_wall.lastValueSignaled();

        if (current_frame_index > m_rendering_tasks.m_frame_consumed_wall.lastValueSignaled()
            + m_rendering_tasks.m_queued_frames_count)
        {
            YieldProcessor();
            return false;    // we return 'false' here because we want this task to be rescheduled at a later time
        }

        PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
        "CPU job for frame %i start", current_frame_index);

        m_command_list.reset();
        m_command_list.setDescriptorHeaps(m_page0_descriptor_heaps);
        m_command_list.inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle);

        {
            // set rendering targets
            RenderTargetViewDescriptorTable const* p_supplied_rtv_table = m_rendering_tasks.m_color_rendering_target_ptr
                ? &m_rendering_tasks.m_color_rendering_target_ptr->rtvTable() : nullptr;

            DepthStencilViewDescriptorTable const* p_supplied_dsv_table = m_rendering_tasks.m_depth_rendering_target_ptr
                ? &m_rendering_tasks.m_depth_rendering_target_ptr->dsvTable() : nullptr;

            RenderTargetViewDescriptorTable processed_rtv_table{ *p_supplied_rtv_table };
            DepthStencilViewDescriptorTable processed_dsv_table{ *p_supplied_dsv_table };

            uint16_t frame_shift = current_frame_index % m_rendering_tasks.m_queued_frames_count;
            
            uint32_t rtv_shift = processed_rtv_table.descriptor_size*frame_shift;
            processed_rtv_table.cpu_pointer += rtv_shift;
            processed_rtv_table.gpu_pointer += rtv_shift;

            uint32_t dsv_shift = processed_dsv_table.descriptor_size*frame_shift;
            processed_dsv_table.cpu_pointer += dsv_shift;
            processed_dsv_table.gpu_pointer += dsv_shift;


            m_command_list.outputMergerSetRenderTargets(
                m_rendering_tasks.m_color_rendering_target_ptr
                ? &processed_rtv_table
                : nullptr, 1,
                m_rendering_tasks.m_depth_rendering_target_ptr
                ? &processed_dsv_table
                : nullptr,
                0U);

            m_rendering_tasks.m_color_rendering_target_ptr->switchToRenderAccessState(m_command_list);
            m_rendering_tasks.m_depth_rendering_target_ptr->switchToRenderAccessState(m_command_list);

            m_command_list.clearRenderTargetView(processed_rtv_table, 0, m_clear_color);
        }

        m_command_list.close();

        m_rendering_tasks.m_device.defaultCommandQueue().executeCommandList(m_command_list);

        return true;
    }

    TaskType get_task_type() const override
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
    bool do_task(uint8_t worker_id, uint16_t frame_index) override
    {
        m_command_list.reset();

        if(m_rendering_tasks.m_color_rendering_target_ptr)
            m_rendering_tasks.m_color_rendering_target_ptr->switchToInitialState(m_command_list);

        if(m_rendering_tasks.m_depth_rendering_target_ptr)
            m_rendering_tasks.m_depth_rendering_target_ptr->switchToInitialState(m_command_list);

        m_command_list.close();
        m_rendering_tasks.m_device.defaultCommandQueue().executeCommandList(m_command_list);
        m_rendering_tasks.m_end_of_frame_gpu_wall.signalFromGPU(m_rendering_tasks.m_device.defaultCommandQueue());
        
        PIXSetMarker(pix_marker_colors::PixCPUJobMarkerColor,
            "CPU job for frame %i finish", m_rendering_tasks.m_end_of_frame_cpu_wall.lastValueSignaled());

        m_rendering_tasks.m_end_of_frame_cpu_wall.signalFromCPU();

        return true;
    }

    TaskType get_task_type() const override
    {
        return TaskType::gpu_draw;
    }

private:
    RenderingTasks& m_rendering_tasks;
    CommandList m_command_list;
};


RenderingTasks::RenderingTasks(Globals& globals):
    m_dx_resources{ *globals.get<DxResourceFactory>() },
    m_device{ *globals.get<Device>() },
    m_task_graph{ globals.get<GlobalSettings>()->getNumberOfWorkers(), "RenderingTasksGraph" },
    m_task_sink{ m_task_graph, *globals.get<std::vector<std::ostream*>>(), "RenderingTasksSink" },

    m_frame_begin_task{ new FrameBeginTask{*this, math::Vector4f{0.f, 0.f, 0.f, 0.f}} },
    m_frame_end_task{ new FrameEndTask{*this} },

    m_queued_frames_count{ globals.get<GlobalSettings>()->getMaxFramesInFlight() },
    m_end_of_frame_cpu_wall{ m_device },
    m_end_of_frame_gpu_wall{ m_device },
    m_frame_consumed_wall{ m_device }
{
    m_task_graph.setRootNodes({ m_frame_begin_task.get() });
    m_frame_begin_task->addDependent(*m_frame_end_task);
}

RenderingTasks::~RenderingTasks() = default;


void RenderingTasks::run()
{
    m_task_sink.run();
}

void RenderingTasks::dispatchExitSignal()
{
    m_task_sink.dispatchExitSignal();
}

void RenderingTasks::setRenderingTargets(std::shared_ptr<RenderingTargetColor> const& color_rendering_target,
    std::shared_ptr<RenderingTargetDepth> const& depth_rendering_target)
{
    uint32_t color_descriptors_count = color_rendering_target->rtvTable().descriptor_count;
    if (color_descriptors_count != m_queued_frames_count)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
            "Attempted to set invalid color rendering target: the target must have the same number"
            " of elements as the maximal frame queue length (" + std::to_string(m_queued_frames_count)
            + " frames as defined by the current settings). However, the target supplies " 
            + std::to_string(color_descriptors_count) + " elements");
    }

    uint32_t depth_descriptors_count = depth_rendering_target->dsvTable().descriptor_count;
    if (depth_descriptors_count != m_queued_frames_count)
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
            "Attempted to set invalid depth rendering target: the target must have the same number"
            " of elements as the maximal frame queue length (" + std::to_string(m_queued_frames_count)
            + "frames as defined by the current settings). However, the target supplies " 
            + std::to_string(depth_descriptors_count) + " elements");
    }

    m_color_rendering_target_ptr = color_rendering_target;
    m_depth_rendering_target_ptr = depth_rendering_target;
}

void RenderingTasks::consumeFrame()
{
    m_frame_consumed_wall.signalFromCPU();
}

void RenderingTasks::waitUntilFrameIsReady(uint64_t frame_index) const
{
    m_end_of_frame_gpu_wall.waitUntilValue(frame_index);
}

uint64_t RenderingTasks::totalFramesScheduled() const
{
    return m_end_of_frame_cpu_wall.lastValueSignaled();
}

uint64_t RenderingTasks::totalFramesRendered() const
{
    return m_end_of_frame_gpu_wall.lastValueSignaled();
}

uint64_t RenderingTasks::totalFramesConsumed() const
{
    return m_frame_consumed_wall.lastValueSignaled();
}

