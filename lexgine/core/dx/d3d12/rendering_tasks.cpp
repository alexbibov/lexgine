#include "rendering_tasks.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/global_settings.h"
#include "lexgine/core/concurrency/schedulable_task.h"

#include "rendering_loop.h"
#include "dx_resource_factory.h"
#include "device.h"
#include "command_list.h"
#include "lexgine/core/math/vector_types.h"


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

        uint64_t active_color_targets_mask = m_rendering_tasks.m_color_rendering_target_ptr->activeColorTargetsMask();

        m_command_list.reset();
        m_command_list.setDescriptorHeaps(m_page0_descriptor_heaps);
        m_command_list.inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle);
        m_command_list.outputMergerSetRenderTargets(
            m_rendering_tasks.m_color_rendering_target_ptr
            ? &m_rendering_tasks.m_color_rendering_target_ptr->rtvTable()
            : nullptr, active_color_targets_mask,
            m_rendering_tasks.m_depth_rendering_target_ptr
            ? &m_rendering_tasks.m_depth_rendering_target_ptr->dsvTable()
            : nullptr,
            0U);
        m_rendering_tasks.m_color_rendering_target_ptr->switchToRenderAccessState(m_command_list);
        m_rendering_tasks.m_depth_rendering_target_ptr->switchToRenderAccessState(m_command_list);

        {
            unsigned long idx{ 0 };
            for (unsigned long offset = 0;
                _BitScanForward64(&idx, active_color_targets_mask);
                offset += idx, active_color_targets_mask >>= idx + 1)
            {
                m_command_list.clearRenderTargetView(m_rendering_tasks.m_color_rendering_target_ptr->rtvTable(),
                    static_cast<uint32_t>(offset), m_clear_color);
            }
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
        uint64_t active_color_targets_mask = m_rendering_tasks.m_color_rendering_target_ptr->activeColorTargetsMask();

        m_command_list.reset();
        m_rendering_tasks.m_color_rendering_target_ptr->switchToInitialState(m_command_list);
        m_rendering_tasks.m_depth_rendering_target_ptr->switchToInitialState(m_command_list);
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
    CommandList m_command_list;
};


RenderingTasks::RenderingTasks(Globals& globals):
    m_dx_resources{ *globals.get<DxResourceFactory>() },
    m_device{ *globals.get<Device>() },
    m_task_graph{ globals.get<GlobalSettings>()->getNumberOfWorkers(), "RenderingTasksGraph" },
    m_task_sink{ m_task_graph, *globals.get<std::vector<std::ostream*>>(), "RenderingTasksSink" },

    m_frame_begin_task{ new FrameBeginTask{*this, math::Vector4f{0.f, 0.f, 0.f, 0.f}} },
    m_frame_end_task{ new FrameEndTask{*this} }

{
    m_task_graph.setRootNodes({ m_frame_begin_task.get() });
    m_frame_begin_task->addDependent(*m_frame_end_task);
}

RenderingTasks::~RenderingTasks() = default;


void RenderingTasks::run()
{
    m_task_sink.run();
}

void RenderingTasks::setRenderingTargets(RenderingTargetColor const* color_rendering_target, 
    RenderingTargetDepth const* depth_rendering_target)
{
    m_color_rendering_target_ptr = color_rendering_target;
    m_depth_rendering_target_ptr = depth_rendering_target;
}

