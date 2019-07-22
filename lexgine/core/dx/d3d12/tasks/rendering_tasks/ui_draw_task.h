#ifndef LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_UI_DRAW_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_UI_DRAW_TASK_H

#include "3rd_party/imgui/imgui.h"

#include "lexgine/core/lexgine_core_fwd.h"
#include "lexgine/core/ui/lexgine_core_ui_fwd.h"
#include "lexgine/core/dx/d3d12/pipeline_state.h"
#include "lexgine/core/dx/d3d12/rendering_work.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"

#include "lexgine/core/dx/d3d12/resource_data_uploader.h"
#include "lexgine/core/dx/d3d12/vertex_buffer_binding.h"
#include "lexgine/core/concurrency/schedulable_task.h"
#include "lexgine/osinteraction/windows/lexgine_osinteraction_windows_fwd.h"
#include "lexgine/osinteraction/listener.h"
#include "lexgine/osinteraction/windows/window_listeners.h"

#include "gpu_work_execution_task.h"


namespace lexgine::core::dx::d3d12::tasks::rendering_tasks {

class UIDrawTask :
    public RenderingWork,
    public GpuWorkSource,
    public osinteraction::Listeners<
    osinteraction::windows::KeyInputListener,
    osinteraction::windows::MouseButtonListener,
    osinteraction::windows::MouseMoveListener,
    osinteraction::windows::CursorUpdateListener
    >,
    public std::enable_shared_from_this<UIDrawTask>
{
    using lexgine::core::Entity::getId;

public:
    static std::shared_ptr<UIDrawTask> create(Globals& globals, BasicRenderingServices& basic_rendering_services);

public:
    ~UIDrawTask();

    void addUIProvider(std::weak_ptr<ui::UIProvider> const& ui_provider) { m_ui_providers.push_back(ui_provider); }

public:    // RenderingWork interface
    void updateRenderingConfiguration(RenderingConfigurationUpdateFlags update_flags,
        RenderingConfiguration const& rendering_configuration) override;

public:    // KeyInputListener events
    bool keyDown(osinteraction::SystemKey key) override;
    bool keyUp(osinteraction::SystemKey key) override;
    bool character(wchar_t char_key) override;
    bool systemKeyDown(osinteraction::SystemKey key) override;
    bool systemKeyUp(osinteraction::SystemKey key) override;

public:    // MouseButtonListener events
    bool buttonDown(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;
    bool buttonUp(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;
    bool doubleClick(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;
    bool wheelMove(double move_delta, bool is_horizontal_wheel, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;

public:    // MouseMoveListener events
    bool move(uint16_t x, uint16_t y, osinteraction::windows::ControlKeyFlag const& control_key_flag) override;
    bool enter_client_area() override;
    bool leave_client_area() override;

public:    // CursorUpdateListener events
    bool setCursor() override;

private:
    UIDrawTask(Globals& globals, BasicRenderingServices& basic_rendering_services);

private:    // required by AbstractTask interface
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override { return concurrency::TaskType::cpu; }

private:
    void processEvents() const;
    void drawFrame();

private:
    Globals& m_globals;
    Device& m_device;
    BasicRenderingServices& m_basic_rendering_services;
    osinteraction::windows::Window* m_rendering_window_ptr = nullptr;
    mutable long long m_time_counter;
    mutable ImGuiMouseCursor m_mouse_cursor;
    ResourceDataUploader m_resource_uploader;

    tasks::RootSignatureCompilationTask* m_rs = nullptr;
    tasks::HLSLCompilationTask* m_vs = nullptr;
    tasks::HLSLCompilationTask* m_ps = nullptr;
    tasks::GraphicsPSOCompilationTask* m_pso = nullptr;
    VertexAttributeSpecificationList m_va_list;
    GraphicsPSODescriptor m_pso_desc;

    std::vector<uint32_t> m_projection_matrix_constants;

    ImGuiContext* m_gui_context = nullptr;

    std::unique_ptr<CommittedResource> m_fonts_texture;
    ShaderResourceDescriptorTable m_srv_table;

    std::unique_ptr<PerFrameUploadDataStreamAllocator> m_ui_data_allocator;
    std::unique_ptr<VertexBufferBinding> m_ui_vertex_data_binding;
    std::unique_ptr<IndexBufferBinding> m_ui_index_data_binding;

    PerFrameUploadDataStreamAllocator::address_type m_vertex_and_index_data_allocation = nullptr;

    misc::StaticVector<Viewport, CommandList::c_maximal_viewport_count> m_viewports;
    misc::StaticVector<math::Rectangle, CommandList::c_maximal_scissor_rectangle_count> m_scissor_rectangles;

    std::list<std::weak_ptr<ui::UIProvider>> m_ui_providers;

private:
    static std::string const c_interface_update_section;
    static size_t const c_interface_update_section_size = 1024 * 1024 * 8;
};

}

#endif
