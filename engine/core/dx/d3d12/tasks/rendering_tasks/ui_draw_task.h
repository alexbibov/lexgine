#ifndef LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_UI_DRAW_TASK_H
#define LEXGINE_CORE_DX_D3D12_TASKS_RENDERING_TASKS_UI_DRAW_TASK_H

#include <chrono>

#include "imgui.h"

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/ui/lexgine_core_ui_fwd.h"
#include "engine/core/dx/d3d12/pipeline_state.h"
#include "engine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "engine/core/dx/d3d12/tasks/lexgine_core_dx_d3d12_tasks_fwd.h"

#include "engine/core/dx/d3d12/resource_data_uploader.h"
#include "engine/core/dx/d3d12/vertex_buffer_binding.h"
#include "engine/core/dx/d3d12/constant_buffer_data_mapper.h"

#include "engine/core/dx/dxcompilation/lexgine_core_dx_dxcompilation_fwd.h"
#include "engine/core/concurrency/schedulable_task.h"
#include "engine/osinteraction/windows/lexgine_osinteraction_windows_fwd.h"
#include "engine/osinteraction/listener.h"
#include "engine/osinteraction/windows/window_listeners.h"
#include "engine/core/dx/dxcompilation/shader_function.h"

#include "rendering_work.h"

namespace lexgine::core::dx::d3d12::tasks::rendering_tasks {

enum class MouseTrackedArea
{
	none,
	client,
	nonclient
};
struct OsInterationData
{
	std::chrono::high_resolution_clock::time_point system_time;
	ImGuiMouseCursor last_mouse_cursor;
	osinteraction::windows::Window* rendering_window_ptr;
	uint32_t keyboard_code_page;
	MouseTrackedArea mouse_tracked_area;

	OsInterationData()
		: system_time{ std::chrono::high_resolution_clock::now() }
		, last_mouse_cursor{ ImGuiMouseCursor_COUNT }
		, rendering_window_ptr{ nullptr }
		, keyboard_code_page{ std::numeric_limits<uint32_t>::max() }
		, mouse_tracked_area{ MouseTrackedArea::none }
	{
	}
};

class UIDrawTask final :
    public RenderingWork,
    public osinteraction::Listeners<
    osinteraction::windows::KeyInputListener,
    osinteraction::windows::MouseButtonListener,
    osinteraction::windows::MouseMoveListener,
    osinteraction::windows::CursorUpdateListener,
    osinteraction::windows::FocusUpdateListener,
    osinteraction::windows::InputLanguageChangeListener
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
    bool character(uint64_t char_key_code) override;
    bool systemKeyDown(osinteraction::SystemKey key) override;
    bool systemKeyUp(osinteraction::SystemKey key) override;

public:    // MouseButtonListener events
    bool buttonDown(osinteraction::MouseButton button, uint16_t xbutton_id, osinteraction::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;
    bool buttonUp(osinteraction::MouseButton button, uint16_t xbutton_id, osinteraction::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;
    bool doubleClick(osinteraction::MouseButton button, uint16_t xbutton_id, osinteraction::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;
    bool wheelMove(double move_delta, bool is_horizontal_wheel, osinteraction::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y) override;

public:    // MouseMoveListener events
    bool move(uint16_t x, uint16_t y, osinteraction::ControlKeyFlag const& control_key_flag) override;
    bool enter_client_area() override;
    bool leave_client_area() override;

public:    // CursorUpdateListener events
    bool setCursor(uint64_t wparam, uint64_t lparam) override;

public:    // FocusUpdateListener
    bool setFocus(uint64_t param) override;
    bool killFocus(uint64_t param) override;

public:    // InputLanguageChangedListener
    bool inputLanguageChanged() override;

private:
    UIDrawTask(Globals& globals, BasicRenderingServices& basic_rendering_services);

private:    // required by AbstractTask interface
    bool doTask(uint8_t worker_id, uint64_t user_data) override;
    concurrency::TaskType type() const override { return concurrency::TaskType::cpu; }

private:
    void updateTexture(ImTextureData* p_texture);
    void processEvents();
    void drawFrame();

private:
    Globals& m_globals;
    Device& m_device;
    BasicRenderingServices& m_basic_rendering_services;
    ResourceDataUploader& m_resource_uploader;

    ImGuiContext* m_im_gui_context = nullptr;
    OsInterationData m_os_data;

    tasks::HLSLCompilationTask* m_vs = nullptr;
    tasks::HLSLCompilationTask* m_ps = nullptr;
    tasks::GraphicsPSOCompilationTask* m_pso = nullptr;
    VertexAttributeSpecificationList m_va_list;
    dxcompilation::ShaderFunction m_shader_function;
    GraphicsPSODescriptor m_pso_desc;

    ConstantBufferReflection m_constant_buffer_reflection;
    ConstantBufferDataMapper m_constant_data_mapper;
    math::Matrix4f m_projection_matrix;

    std::unordered_map<ImTextureID, std::unique_ptr<CommittedResource>> m_imgui_textures;
    ImTextureID m_currentlyBoundImGuiTextureId{ ImTextureID_Invalid };

    std::unique_ptr<VertexBufferBinding> m_ui_vertex_data_binding;
    std::unique_ptr<IndexBufferBinding> m_ui_index_data_binding;

    PerFrameUploadDataStreamAllocator& m_ui_data_allocator;
    PerFrameUploadDataStreamAllocator::address_type m_vertex_and_index_data_allocation = nullptr;

    misc::StaticVector<Viewport, CommandList::c_maximal_viewport_count> m_viewports;
    misc::StaticVector<math::Rectangle, CommandList::c_maximal_scissor_rectangle_count> m_scissor_rectangles;

    std::list<std::weak_ptr<ui::UIProvider>> m_ui_providers;

    CommandList* m_cmd_list_ptr = nullptr;

private:
    static std::string const c_interface_update_section;
    static size_t const c_interface_update_section_size = 1024 * 1024 * 8;
    tasks::RootSignatureCompilationTask* m_rs;
};

}

#endif
