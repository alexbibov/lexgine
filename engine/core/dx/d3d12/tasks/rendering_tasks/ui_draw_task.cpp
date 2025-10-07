#include "engine/core/exception.h"
#include "engine/core/globals.h"
#include "engine/core/engine_api.h"
#include "engine/core/profiling_services.h"
#include "engine/core/rendering_configuration.h"
#include "engine/core/ui/ui_provider.h"
#include "engine/core/dx/d3d12/dx_resource_factory.h"
#include "engine/core/dx/d3d12/device.h"
#include "engine/core/dx/d3d12/resource.h"
#include "engine/core/dx/d3d12/resource_barrier_pack.h"
#include "engine/core/dx/d3d12/basic_rendering_services.h"
#include "engine/core/dx/d3d12/descriptor_table_builders.h"
#include "engine/core/dx/d3d12/tasks/root_signature_compilation_task.h"
#include "engine/core/dx/d3d12/tasks/pso_compilation_task.h"
#include "engine/core/dx/d3d12/tasks/hlsl_compilation_task.h"
#include "engine/core/dx/dxcompilation/shader_stage.h"
#include "engine/core/math/utility.h"
#include "ui_draw_task.h"


using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;


namespace {

static const std::unordered_map<osinteraction::SystemKey, ImGuiKey> systemKeyToImGuiLUT =
{
	{ osinteraction::SystemKey::tab, ImGuiKey_Tab },
	{ osinteraction::SystemKey::caps, ImGuiKey_CapsLock },
	{ osinteraction::SystemKey::lshift, ImGuiKey_LeftShift },
	{ osinteraction::SystemKey::lctrl, ImGuiKey_LeftCtrl },
	{ osinteraction::SystemKey::lalt, ImGuiKey_LeftAlt },
	{ osinteraction::SystemKey::backspace, ImGuiKey_Backspace },
	{ osinteraction::SystemKey::enter, ImGuiKey_Enter },
	{ osinteraction::SystemKey::keypad_enter, ImGuiKey_KeypadEnter },
	{ osinteraction::SystemKey::rshift, ImGuiKey_RightShift },
	{ osinteraction::SystemKey::rctrl, ImGuiKey_RightCtrl },
	{ osinteraction::SystemKey::ralt, ImGuiKey_RightAlt },
	{ osinteraction::SystemKey::esc, ImGuiKey_Escape },

	{ osinteraction::SystemKey::f1, ImGuiKey_F1 },
	{ osinteraction::SystemKey::f2, ImGuiKey_F2 },
	{ osinteraction::SystemKey::f3, ImGuiKey_F3 },
	{ osinteraction::SystemKey::f4, ImGuiKey_F4 },
	{ osinteraction::SystemKey::f5, ImGuiKey_F5 },
	{ osinteraction::SystemKey::f6, ImGuiKey_F6 },
	{ osinteraction::SystemKey::f7, ImGuiKey_F7 },
	{ osinteraction::SystemKey::f8, ImGuiKey_F8 },
	{ osinteraction::SystemKey::f9, ImGuiKey_F9 },
	{ osinteraction::SystemKey::f10, ImGuiKey_F10 },
	{ osinteraction::SystemKey::f11, ImGuiKey_F11 },
	{ osinteraction::SystemKey::f12, ImGuiKey_F12 },

	{ osinteraction::SystemKey::print_screen, ImGuiKey_PrintScreen },
	{ osinteraction::SystemKey::scroll_lock, ImGuiKey_ScrollLock },
	{ osinteraction::SystemKey::num_lock, ImGuiKey_NumLock },
	{ osinteraction::SystemKey::pause, ImGuiKey_Pause },

	{ osinteraction::SystemKey::insert, ImGuiKey_Insert },
	{ osinteraction::SystemKey::home, ImGuiKey_Home },
	{ osinteraction::SystemKey::page_up, ImGuiKey_PageUp },
	{ osinteraction::SystemKey::_delete, ImGuiKey_Delete },
	{ osinteraction::SystemKey::end, ImGuiKey_End },
	{ osinteraction::SystemKey::page_down, ImGuiKey_PageDown },

	{ osinteraction::SystemKey::arrow_left, ImGuiKey_LeftArrow },
	{ osinteraction::SystemKey::arrow_right, ImGuiKey_RightArrow },
	{ osinteraction::SystemKey::arrow_down, ImGuiKey_DownArrow },
	{ osinteraction::SystemKey::arrow_up, ImGuiKey_UpArrow },

	{ osinteraction::SystemKey::space, ImGuiKey_Space },

	// Numpad
	{ osinteraction::SystemKey::num_0, ImGuiKey_Keypad0 },
	{ osinteraction::SystemKey::num_1, ImGuiKey_Keypad1 },
	{ osinteraction::SystemKey::num_2, ImGuiKey_Keypad2 },
	{ osinteraction::SystemKey::num_3, ImGuiKey_Keypad3 },
	{ osinteraction::SystemKey::num_4, ImGuiKey_Keypad4 },
	{ osinteraction::SystemKey::num_5, ImGuiKey_Keypad5 },
	{ osinteraction::SystemKey::num_6, ImGuiKey_Keypad6 },
	{ osinteraction::SystemKey::num_7, ImGuiKey_Keypad7 },
	{ osinteraction::SystemKey::num_8, ImGuiKey_Keypad8 },
	{ osinteraction::SystemKey::num_9, ImGuiKey_Keypad9 },
	{ osinteraction::SystemKey::multiply, ImGuiKey_KeypadMultiply },
	{ osinteraction::SystemKey::divide, ImGuiKey_KeypadDivide },
	{ osinteraction::SystemKey::add, ImGuiKey_KeypadAdd },
	{ osinteraction::SystemKey::subtract, ImGuiKey_KeypadSubtract },
	{ osinteraction::SystemKey::decimal, ImGuiKey_KeypadDecimal },

	// Digits
	{ osinteraction::SystemKey::_0, ImGuiKey_0 },
	{ osinteraction::SystemKey::_1, ImGuiKey_1 },
	{ osinteraction::SystemKey::_2, ImGuiKey_2 },
	{ osinteraction::SystemKey::_3, ImGuiKey_3 },
	{ osinteraction::SystemKey::_4, ImGuiKey_4 },
	{ osinteraction::SystemKey::_5, ImGuiKey_5 },
	{ osinteraction::SystemKey::_6, ImGuiKey_6 },
	{ osinteraction::SystemKey::_7, ImGuiKey_7 },
	{ osinteraction::SystemKey::_8, ImGuiKey_8 },
	{ osinteraction::SystemKey::_9, ImGuiKey_9 },

	// Letters
	{ osinteraction::SystemKey::A, ImGuiKey_A },
	{ osinteraction::SystemKey::B, ImGuiKey_B },
	{ osinteraction::SystemKey::C, ImGuiKey_C },
	{ osinteraction::SystemKey::D, ImGuiKey_D },
	{ osinteraction::SystemKey::E, ImGuiKey_E },
	{ osinteraction::SystemKey::F, ImGuiKey_F },
	{ osinteraction::SystemKey::G, ImGuiKey_G },
	{ osinteraction::SystemKey::H, ImGuiKey_H },
	{ osinteraction::SystemKey::I, ImGuiKey_I },
	{ osinteraction::SystemKey::J, ImGuiKey_J },
	{ osinteraction::SystemKey::K, ImGuiKey_K },
	{ osinteraction::SystemKey::L, ImGuiKey_L },
	{ osinteraction::SystemKey::M, ImGuiKey_M },
	{ osinteraction::SystemKey::N, ImGuiKey_N },
	{ osinteraction::SystemKey::O, ImGuiKey_O },
	{ osinteraction::SystemKey::P, ImGuiKey_P },
	{ osinteraction::SystemKey::Q, ImGuiKey_Q },
	{ osinteraction::SystemKey::R, ImGuiKey_R },
	{ osinteraction::SystemKey::S, ImGuiKey_S },
	{ osinteraction::SystemKey::T, ImGuiKey_T },
	{ osinteraction::SystemKey::U, ImGuiKey_U },
	{ osinteraction::SystemKey::V, ImGuiKey_V },
	{ osinteraction::SystemKey::W, ImGuiKey_W },
	{ osinteraction::SystemKey::X, ImGuiKey_X },
	{ osinteraction::SystemKey::Y, ImGuiKey_Y },
	{ osinteraction::SystemKey::Z, ImGuiKey_Z },

	// Mouse buttons
	{ osinteraction::SystemKey::mouse_left_button, ImGuiKey_MouseLeft },
	{ osinteraction::SystemKey::mouse_right_button, ImGuiKey_MouseRight },
	{ osinteraction::SystemKey::mouse_middle_button, ImGuiKey_MouseMiddle },
	{ osinteraction::SystemKey::mouse_x_button_1, ImGuiKey_MouseX1 },
	{ osinteraction::SystemKey::mouse_x_button_2, ImGuiKey_MouseX2 }
};

bool isVkDownWin32(int vk_key)
{
    return (GetKeyState(vk_key) & 0x8000) != 0;
}

void updateKeyModifiersWin32(ImGuiIO& io)
{
    io.AddKeyEvent(ImGuiMod_Ctrl, isVkDownWin32(VK_CONTROL));
    io.AddKeyEvent(ImGuiMod_Shift, isVkDownWin32(VK_SHIFT));
    io.AddKeyEvent(ImGuiMod_Alt, isVkDownWin32(VK_MENU));
    io.AddKeyEvent(ImGuiMod_Super, isVkDownWin32(VK_LWIN | VK_RWIN));
}

bool processKeyEventWin32(ImGuiIO& io, osinteraction::SystemKey key, bool is_key_down)
{
	if (!systemKeyToImGuiLUT.contains(key))
	{
		return false;
	}
	updateKeyModifiersWin32(io);
	ImGuiKey imgui_key = systemKeyToImGuiLUT.at(key);
	io.AddKeyEvent(imgui_key, is_key_down);
    return true;
}

void updateKeyboardCodepageWin32(ImGuiIO& io, uint32_t& keyboard_code_page)
{
	// Configure keyboard layout
	HKL keyboard_layout = GetKeyboardLayout(0);
	LCID keyboard_lcid = MAKELCID(LOWORD(keyboard_layout), SORT_DEFAULT);
	if (GetLocaleInfo(
		keyboard_lcid,
		LOCALE_RETURN_NUMBER | LOCALE_IDEFAULTANSICODEPAGE,
		reinterpret_cast<LPWSTR>(&keyboard_code_page),
		sizeof(keyboard_code_page)) == 0)
	{
		keyboard_code_page = CP_ACP;
	}
}

bool mouseButtonHandler(osinteraction::MouseButton button, uint16_t xbutton_id,
    osinteraction::windows::Window const& window, bool acquire_capture)
{
    ImGuiIO& io = ImGui::GetIO();

    switch (button)
    {
    case osinteraction::MouseButton::left:
        io.MouseDown[0] = acquire_capture;
        break;
    case osinteraction::MouseButton::middle:
        io.MouseDown[2] = acquire_capture;
        break;
    case osinteraction::MouseButton::right:
        io.MouseDown[1] = acquire_capture;
        break;
    case osinteraction::MouseButton::x:
        io.MouseDown[xbutton_id + 2] = acquire_capture;
        break;
    default:
        __assume(0);
    }

    if (acquire_capture)
    {
        if (!ImGui::IsAnyMouseDown() && GetCapture() == NULL)
            SetCapture(window.native());
    }
    else
    {
        if (!ImGui::IsAnyMouseDown() && GetCapture() == window.native())
            ReleaseCapture();
    }

    return true;
}

bool updateMouseCursorWin32(ImGuiIO& io, ImGuiMouseCursor imgui_cursor)
{
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        SetCursor(NULL);
    }
    else
    {
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
        case ImGuiMouseCursor_Wait:         win32_cursor = IDC_WAIT; break;
        case ImGuiMouseCursor_Progress:     win32_cursor = IDC_APPSTARTING; break;
        case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
        }
        SetCursor(LoadCursor(NULL, win32_cursor));
    }
    return true;
}

ImGuiMouseSource getMouseSourceFromMessageExtraInfoWin32()
{
    LPARAM extra_info = GetMessageExtraInfo();
    if ((extra_info & 0xFFFFFF80) == 0xFF515700)
        return ImGuiMouseSource_Pen;
    if ((extra_info & 0xFFFFFF80) == 0xFF515780)
        return ImGuiMouseSource_TouchScreen;
    return ImGuiMouseSource_Mouse;
}

void updateMouseDataWin32(HWND hwnd, MouseTrackedArea mouse_tracked_area)
{
    ImGuiIO& io = ImGui::GetIO();
    HWND focused_window = GetForegroundWindow();
    bool is_focused = hwnd = focused_window;

    if (is_focused)
    {
		if (io.WantSetMousePos)
		{
			POINT pos = { 
                static_cast<int>(io.MousePos.x), 
                static_cast<int>(io.MousePos.y)
            };
			ClientToScreen(hwnd, &pos);
			SetCursorPos(pos.x, pos.y);
		}

        if (!io.WantSetMousePos && mouse_tracked_area == MouseTrackedArea::none)
        {
			POINT pos;
            if (GetCursorPos(&pos) && ScreenToClient(hwnd, &pos))
            {
                io.AddMousePosEvent(static_cast<float>(pos.x), static_cast<float>(pos.y));
            }
        }
    }
}

}

std::shared_ptr<UIDrawTask> UIDrawTask::create(Globals& globals, BasicRenderingServices& basic_rendering_services)
{
    std::shared_ptr<UIDrawTask> rv{ new UIDrawTask{globals,  basic_rendering_services} };
    return rv;
}

UIDrawTask::~UIDrawTask()
{
    ImGui::DestroyContext();
}


void UIDrawTask::updateRenderingConfiguration(RenderingConfigurationUpdateFlags update_flags,
    RenderingConfiguration const& rendering_configuration)
{
    if (update_flags.isSet(RenderingConfigurationUpdateFlags::base_values::rendering_window_changed))
    {
        m_os_data.rendering_window_ptr = rendering_configuration.p_rendering_window;
        m_os_data.rendering_window_ptr->addListener(shared_from_this());
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        main_viewport->PlatformHandle = m_os_data.rendering_window_ptr->native();
    }

    if (update_flags.isSet(RenderingConfigurationUpdateFlags::base_values::color_format_changed)
        || update_flags.isSet(RenderingConfigurationUpdateFlags::base_values::depth_format_changed))
    {
        task_caches::PSOCompilationTaskCache& pso_compilation_task_cache = *m_globals.get<task_caches::PSOCompilationTaskCache>();

        if (!m_pso)
        {
            m_pso_desc.node_mask = 0x1;
            m_pso_desc.vertex_attributes = m_va_list;
            m_pso_desc.primitive_topology_type = PrimitiveTopologyType::triangle;
            m_pso_desc.num_render_targets = 1;
            m_pso_desc.rtv_formats[0] = rendering_configuration.color_buffer_format;
            m_pso_desc.dsv_format = rendering_configuration.depth_buffer_format;

            // blending state
            {
                BlendState blending_state;
                BlendDescriptor blending_descriptor{ BlendFactor::source_alpha, BlendFactor::one_minus_source_alpha,
                BlendFactor::one_minus_source_alpha, BlendFactor::zero, BlendOperation::add, BlendOperation::add };
                blending_state.render_target_blend_descriptor[0] = blending_descriptor;

                m_pso_desc.blend_state = blending_state;
            }

            // rasterizer state
            {
                RasterizerDescriptor rasterizer_descriptor{ FillMode::solid, CullMode::none, FrontFaceWinding::clockwise };
                m_pso_desc.rasterization_descriptor = rasterizer_descriptor;
            }

            // depth-stencil state
            {
                DepthStencilDescriptor depth_stencil_descriptor{ false, true, ComparisonFunction::always };
                m_pso_desc.depth_stencil_descriptor = depth_stencil_descriptor;
            }

            m_pso = pso_compilation_task_cache.findOrCreateTask(m_globals, m_pso_desc,
                "ui_rendering_pso__" + std::to_string(rendering_configuration.color_buffer_format)
                + "__" + std::to_string(rendering_configuration.depth_buffer_format), 0);
            m_pso->setVertexShaderCompilationTask(m_vs);
            m_pso->setPixelShaderCompilationTask(m_ps);
            m_pso->setRootSignatureCompilationTask(m_shader_function.buildBindingSignature());
        }
        else
        {
            m_pso_desc.rtv_formats[0] = rendering_configuration.color_buffer_format;
            m_pso_desc.dsv_format = rendering_configuration.depth_buffer_format;
            m_pso = pso_compilation_task_cache.findOrCreateTask(m_globals, m_pso_desc,
                "ui_rendering_pso__" + std::to_string(rendering_configuration.color_buffer_format)
                + "__" + std::to_string(rendering_configuration.depth_buffer_format), 0);
        }
        m_pso->execute(0);
    }
}

bool UIDrawTask::keyDown(osinteraction::SystemKey key)
{
    systemKeyDown(key);
    return true;
}

bool UIDrawTask::keyUp(osinteraction::SystemKey key)
{
    systemKeyUp(key);
    return true;
}

bool UIDrawTask::character(uint64_t char_key_code)
{
    ImGuiIO& io = ImGui::GetIO();

    // char_key_code is wParam in case of Windows OS
    if (IsWindowUnicode(m_os_data.rendering_window_ptr->native()))
    {
        if (char_key_code > 0 && char_key_code < 0x10000)
        {
            io.AddInputCharacterUTF16(static_cast<unsigned short>(char_key_code));
        }
    }
    else
    {
        wchar_t wch = 0;
        MultiByteToWideChar(
            m_os_data.keyboard_code_page,
            MB_PRECOMPOSED,
            reinterpret_cast<char*>(&char_key_code),
            1,
            &wch,
            1
        );
        io.AddInputCharacter(wch);
    }
    return true;
}

bool UIDrawTask::systemKeyDown(osinteraction::SystemKey key)
{
    return processKeyEventWin32(ImGui::GetIO(), key, true);
}

bool UIDrawTask::systemKeyUp(osinteraction::SystemKey key)
{
    ImGuiIO& io = ImGui::GetIO();
    if (key == osinteraction::SystemKey::print_screen)
    {
        // Special behavior for VK_SNAPSHOT / ImGuiKey_PrintScreen as Windows doesn't emit the key down event.
        if (!processKeyEventWin32(io, osinteraction::SystemKey::print_screen, true))
        {
            return false;
        }
    }
    return processKeyEventWin32(io, key, false);
}


bool UIDrawTask::buttonDown(osinteraction::MouseButton button, uint16_t xbutton_id, osinteraction::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    return mouseButtonHandler(button, xbutton_id, *m_os_data.rendering_window_ptr, true);
}

bool UIDrawTask::buttonUp(osinteraction::MouseButton button, uint16_t xbutton_id, osinteraction::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    return mouseButtonHandler(button, xbutton_id, *m_os_data.rendering_window_ptr, false);
}

bool UIDrawTask::doubleClick(osinteraction::MouseButton button, uint16_t xbutton_id, osinteraction::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    return buttonDown(button, xbutton_id, control_key_flag, x, y);
}

bool UIDrawTask::wheelMove(double move_delta, bool is_horizontal_wheel, osinteraction::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    ImGuiIO& io = ImGui::GetIO();

    if (is_horizontal_wheel)
    {
        io.MouseWheelH += static_cast<float>(move_delta);
    }
    else
    {
        io.MouseWheel += static_cast<float>(move_delta);
    }

    return true;
}


bool UIDrawTask::move(uint16_t x, uint16_t y, osinteraction::ControlKeyFlag const& control_key_flag)
{
    ImGuiMouseSource mouse_source = getMouseSourceFromMessageExtraInfoWin32();
    ImGuiIO& io = ImGui::GetIO();

    io.MousePos = ImVec2{ static_cast<float>(x), static_cast<float>(y) };
    return true;
}

bool UIDrawTask::enter_client_area()
{
    return true;
}

bool UIDrawTask::leave_client_area()
{
    return true;
}

bool UIDrawTask::setCursor(uint64_t/* wparam*/, uint64_t/* lparam*/)
{
    ImGuiIO& io = ImGui::GetIO();
    return updateMouseCursorWin32(io, m_os_data.last_mouse_cursor);
}
 
bool UIDrawTask::setFocus(uint64_t/* param*/)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(true);
    return true;
}

bool UIDrawTask::killFocus(uint64_t/* param*/)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(false);
    return true;
}

bool UIDrawTask::inputLanguageChanged()
{
    ImGuiIO& io = ImGui::GetIO();
    updateKeyboardCodepageWin32(io, m_os_data.keyboard_code_page);
    return true;
}


UIDrawTask::UIDrawTask(Globals& globals, BasicRenderingServices& basic_rendering_services)
    : RenderingWork{ globals, "UI draw task", CommandType::direct }
    , m_globals{ globals }
    , m_device{ *globals.get<Device>() }
    , m_basic_rendering_services{ basic_rendering_services }
    , m_resource_uploader{ basic_rendering_services.resourceDataUploader() }
    , m_shader_function{
        m_globals,
        dxcompilation::ShaderFunctionRootUniformBuffers::base_values::SceneUniforms
    }
    , m_constant_data_mapper{ m_constant_buffer_reflection }
    , m_ui_data_allocator{ basic_rendering_services.dynamicGeometryStream() }
    , m_scissor_rectangles(1)
    , m_cmd_list_ptr{ addCommandList() }
{
    m_viewports.emplace_back(math::Vector2f{ 0.f }, math::Vector2f{ 0.f }, math::Vector2f{ 0.f });

    IMGUI_CHECKVERSION();
    m_im_gui_context = ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "Lexgine";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    updateKeyboardCodepageWin32(io, m_os_data.keyboard_code_page);

    // Setup UI device objects

    // shaders
    {
        static char const* const hlsl_source =
            "struct ConstantBufferDataStruct\n\
            {\n\
                float4x4 ProjectionMatrix;\n\
            };\n \
            ConstantBuffer<ConstantBufferDataStruct> constants : register(b0, space100);\n\
            \n\
            struct VS_INPUT\n\
            {\n\
                float2 pos : POSITION;\n\
                float4 col : COLOR0;\n\
                float2 uv : TEXCOORD0;\n\
            };\n\
            \n\
            struct PS_INPUT\n\
            {\n\
                float4 pos : SV_POSITION;\n\
                float4 col : COLOR0;\n\
                float2 uv : TEXCOORD0;\n\
            };\n\
            \n\
            PS_INPUT VSMain(VS_INPUT input)\n\
            {\n\
                PS_INPUT output;\n\
                output.pos = mul(constants.ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n\
                output.col = input.col;\n\
                output.uv = input.uv;\n\
                return output;\n\
            }\n\
            \n\
            SamplerState sampler0 : register(s0);\n\
            Texture2D<float4> texture0 : register(t0);\n\
            \n\
            float4 PSMain(PS_INPUT input) : SV_Target\n\
            {\n\
                float4 out_col = input.col * texture0.Sample(sampler0, input.uv);\n\
                return out_col;\n\
            }\n";

        task_caches::HLSLCompilationTaskCache& hlsl_compilation_task_cache = *m_globals.get<task_caches::HLSLCompilationTaskCache>();
        task_caches::HLSLSourceTranslationUnit hlsl_translation_unit{ m_globals, "ui_rendering_shader", hlsl_source };

        m_vs = hlsl_compilation_task_cache.findOrCreateTask(hlsl_translation_unit, dxcompilation::ShaderModel::model_61, dxcompilation::ShaderType::vertex, "VSMain");
        m_ps = hlsl_compilation_task_cache.findOrCreateTask(hlsl_translation_unit, dxcompilation::ShaderModel::model_61, dxcompilation::ShaderType::pixel, "PSMain");
        m_vs->execute(0);
        m_ps->execute(0);

        dxcompilation::ShaderStage* p_vs_stage = m_shader_function.createShaderStage(m_vs);
        dxcompilation::ShaderStage* p_ps_stage = m_shader_function.createShaderStage(m_ps);
        p_vs_stage->build();
        p_ps_stage->build();
        m_rs = m_shader_function.buildBindingSignature();
        m_rs->execute(0);

        m_constant_buffer_reflection = p_vs_stage->buildConstantBufferReflection(std::string{ "constants" });
        m_constant_data_mapper.addDataBinding("ProjectionMatrix", m_projection_matrix);

        DescriptorHeap& resource_heap = m_basic_rendering_services.dxResources().retrieveDescriptorHeap(m_device, DescriptorHeapType::cbv_srv_uav, 0);
        DescriptorHeap& sampler_heap = m_basic_rendering_services.dxResources().retrieveDescriptorHeap(m_device, DescriptorHeapType::sampler, 0);

        m_shader_function.assignResourceDescriptors(dxcompilation::ShaderFunction::ShaderInputKind::srv, 0, DescriptorAllocationManager{ resource_heap });
        m_shader_function.assignResourceDescriptors(dxcompilation::ShaderFunction::ShaderInputKind::sampler, 0, DescriptorAllocationManager{ sampler_heap });

        // p_ps_stage->bindTexture(std::string{ "texture0" }, *m_fonts_texture);
        p_ps_stage->bindSampler(std::string{ "sampler0" }, FilterPack{ MinificationFilter::linear, MagnificationFilter::linear, 16,
            WrapMode::clamp, WrapMode::clamp, WrapMode::clamp }, math::Vector4f{ 0.f });
    }
    

    // Initialize vertex and index data
    {
        m_ui_vertex_data_binding = std::make_unique<VertexBufferBinding>();

        IndexDataType index_data_type{};
        switch (sizeof(ImDrawIdx))
        {
        case 2:
            index_data_type = IndexDataType::_16_bit;
            break;

        case 4:
            index_data_type = IndexDataType::_32_bit;
            break;

        default:
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, "The actual version of ImGui appears to use unsupported size for the index data");
        }
        m_ui_index_data_binding = std::make_unique<IndexBufferBinding>(m_ui_data_allocator.getUploadResource(), 0, index_data_type, 0);
    }

    // Create input layout
    {
        std::shared_ptr<AbstractVertexAttributeSpecification> position = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
            std::make_shared<VertexAttributeSpecification<float, 2>>(0, static_cast<unsigned int>(offsetof(ImDrawVert, pos)), "POSITION", 0, 0)
            );
        std::shared_ptr<AbstractVertexAttributeSpecification> uv = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
            std::make_shared<VertexAttributeSpecification<float, 2>>(0, static_cast<unsigned int>(offsetof(ImDrawVert, uv)), "TEXCOORD", 0, 0)
            );
        std::shared_ptr<AbstractVertexAttributeSpecification> color = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
            std::make_shared<VertexAttributeSpecification<uint8_t, 4>>(0, static_cast<unsigned int>(offsetof(ImDrawVert, col)), "COLOR", 0, 0)
            );
        m_va_list = VertexAttributeSpecificationList{ position, uv, color };
    }
}

bool UIDrawTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    if (!m_os_data.rendering_window_ptr) return true;

    processEvents();
    ImGui::NewFrame();

    for (auto& ui_provider : m_ui_providers)
    {
        if (auto ui = ui_provider.lock())
        {
            if (ui->isEnabled())
            {
                ui->constructUI();
            }
        }
    }

    ImGui::Render();
    drawFrame();

    return true;
}

void UIDrawTask::updateTexture(ImTextureData* p_texture)
{
	if (p_texture->Status == ImTextureStatus_WantCreate)
	{
		// Create and upload new texture to graphics system
		//IMGUI_DEBUG_LOG("UpdateTexture #%03d: WantCreate %dx%d\n", tex->UniqueID, tex->Width, tex->Height);
		assert(p_texture->TexID == ImTextureID_Invalid && p_texture->BackendUserData == nullptr);
        assert(p_texture->Format == ImTextureFormat_RGBA32);
        auto backend_tex = std::make_unique<CommittedResource>(
            m_device,
            ResourceState::base_values::common,
            misc::makeEmptyOptional<ResourceOptimizedClearValue>(),
            ResourceDescriptor::CreateTexture2D(p_texture->Width, p_texture->Height, 1, DXGI_FORMAT_R8G8B8A8_UNORM),
            AbstractHeapType::_default,
            HeapCreationFlags::base_values::allow_all
        );

		// Store identifiers
		p_texture->SetTexID(reinterpret_cast<ImTextureID>(backend_tex.get()));
        m_imgui_textures[p_texture->GetTexID()] = std::move(backend_tex);
	}

	if (p_texture->Status == ImTextureStatus_WantCreate || p_texture->Status == ImTextureStatus_WantUpdates)
	{
        assert(p_texture->Format == ImTextureFormat_RGBA32);

	    uint32_t const upload_x = (p_texture->Status == ImTextureStatus_WantCreate) ? 0 : p_texture->UpdateRect.x;
        uint32_t const upload_y = (p_texture->Status == ImTextureStatus_WantCreate) ? 0 : p_texture->UpdateRect.y;
        uint32_t const upload_w = (p_texture->Status == ImTextureStatus_WantCreate) ? p_texture->Width : p_texture->UpdateRect.w;
        uint32_t const upload_h = (p_texture->Status == ImTextureStatus_WantCreate) ? p_texture->Height : p_texture->UpdateRect.h;

        size_t upload_pitch_src = p_texture->Width * p_texture->BytesPerPixel;
        size_t upload_row_size_src = upload_w * p_texture->BytesPerPixel;

        ResourceDataUploader& resourceDataUploader = m_basic_rendering_services.resourceDataUploader();

        ResourceDataUploader::TextureSourceDescriptor source_desc{};
        source_desc.subresources.push_back({ .p_data = p_texture->GetPixelsAt(upload_x, upload_y), .row_pitch = upload_pitch_src, .row_size = upload_row_size_src, .slice_pitch = 0 });
        
        ResourceDataUploader::DestinationDescriptor destination_desc{};
        destination_desc.p_destination_resource = m_imgui_textures[p_texture->GetTexID()].get();
        destination_desc.destination_resource_state = ResourceState::base_values::pixel_shader;
        destination_desc.segment.subresources.first_subresource = 0;
        destination_desc.segment.subresources.num_subresources = 1;
        destination_desc.destination_regions.push_back({ .offset_x = upload_x, .offset_y = upload_y, .offset_z = 0, .width = upload_w, .height = upload_h, .depth = 1 });

        resourceDataUploader.addResourceForUpload(destination_desc, source_desc);
        resourceDataUploader.upload();
        resourceDataUploader.waitUntilUploadIsFinished();

		p_texture->SetStatus(ImTextureStatus_OK);
	}

    uint16_t max_frames_in_flight = m_globals.get<GlobalSettings>()->getMaxFramesInFlight();
    if (p_texture->Status == ImTextureStatus_WantDestroy 
        && p_texture->UnusedFrames >= static_cast<int>(max_frames_in_flight))
    {
        m_imgui_textures.erase(p_texture->GetTexID());
    }
}

void UIDrawTask::processEvents()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    RECT rect{};
    GetClientRect(m_os_data.rendering_window_ptr->native(), &rect);
    io.DisplaySize = ImVec2{ 
        static_cast<float>(rect.right - rect.left),
        static_cast<float>(rect.bottom - rect.top)
    };

    std::chrono::time_point current_time = std::chrono::high_resolution_clock::now();
    float ratio = static_cast<float>(std::chrono::nanoseconds::period::num) / std::chrono::nanoseconds::period::den;
    io.DeltaTime = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - m_os_data.system_time).count() * ratio;
    m_os_data.system_time = current_time;

    // Read keyboard modifiers inputs
    io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    io.KeySuper = false;
    // io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.

    // Update OS mouse position
    updateMouseDataWin32(m_os_data.rendering_window_ptr->native(), m_os_data.mouse_tracked_area);

	// ImGui_ImplWin32_ProcessKeyEventsWorkarounds(io);

	ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
	if (m_os_data.last_mouse_cursor != mouse_cursor)
	{
		m_os_data.last_mouse_cursor = mouse_cursor;
		updateMouseCursorWin32(io, mouse_cursor);
	}


    // TODO: support gamepads
}

void UIDrawTask::drawFrame()
{
    if (ImDrawData* p_draw_data = ImGui::GetDrawData())
    {
        auto setup_render_state_lambda = [this, p_draw_data]()
        {
            Viewport viewport{
                math::Vector2f{0.f},
                math::Vector2f{p_draw_data->DisplaySize.x * p_draw_data->FramebufferScale.x, p_draw_data->DisplaySize.y * p_draw_data->FramebufferScale.y },
                math::Vector2f{0.f, 1.f}
            };
            m_viewports[0] = viewport;

            m_projection_matrix = math::createOrthogonalProjectionMatrix(EngineApi::Direct3D12,
                p_draw_data->DisplayPos.x, p_draw_data->DisplayPos.y,
                p_draw_data->DisplaySize.x, p_draw_data->DisplaySize.y, -1.f, 1.f);

            m_cmd_list_ptr->setPipelineState(m_pso->getTaskData());
            m_cmd_list_ptr->setRootSignature(m_rs->getCacheName());

            m_basic_rendering_services.setDefaultResources(*m_cmd_list_ptr);

            m_cmd_list_ptr->rasterizerStateSetViewports(m_viewports);
            m_cmd_list_ptr->inputAssemblySetVertexBuffers(*m_ui_vertex_data_binding);
            m_cmd_list_ptr->inputAssemblySetIndexBuffer(*m_ui_index_data_binding);
            m_cmd_list_ptr->inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle_list);
            m_cmd_list_ptr->outputMergerSetBlendFactor(math::Vector4f{ 0.f });
            m_basic_rendering_services.setDefaultRenderingTarget(*m_cmd_list_ptr);
            auto allocation = m_basic_rendering_services.constantDataStream().allocateAndUpdate(m_constant_data_mapper);
            
            m_shader_function.bindRootConstantBuffer(*m_cmd_list_ptr, dxcompilation::ShaderFunctionConstantBufferRootIds::scene_uniforms, allocation->virtualGpuAddress());
            m_shader_function.bindResourceDescriptors(*m_cmd_list_ptr, dxcompilation::ShaderFunction::ShaderInputKind::srv, 0);
            m_shader_function.bindResourceDescriptors(*m_cmd_list_ptr, dxcompilation::ShaderFunction::ShaderInputKind::sampler, 0);
        };

        // upload vertex data and set rendering context state
        {
            if (p_draw_data->DisplaySize.x <= 0 || p_draw_data->DisplaySize.y <= 0)
                return;

            if (p_draw_data->Textures != nullptr)
            {
                for (ImTextureData* p_texture : *p_draw_data->Textures)
                {
                    if (p_texture->Status != ImTextureStatus_OK)
                    {
                        updateTexture(p_texture);
                    }
                }
            }

            size_t total_vertex_count = p_draw_data->TotalVtxCount + 5000;
            size_t total_index_count = p_draw_data->TotalIdxCount + 10000;
            size_t index_buffer_offset = total_vertex_count * sizeof(ImDrawVert);
            size_t required_draw_buffer_capacity = index_buffer_offset + total_index_count * sizeof(ImDrawIdx);

            m_vertex_and_index_data_allocation = m_ui_data_allocator.allocate(required_draw_buffer_capacity);
            m_ui_vertex_data_binding->setVertexBufferView(0, m_ui_data_allocator.getUploadResource(), m_vertex_and_index_data_allocation->offset(),
                sizeof(ImDrawVert), static_cast<uint32_t>(p_draw_data->TotalVtxCount));
            m_ui_index_data_binding->update(m_vertex_and_index_data_allocation->offset() + total_vertex_count * sizeof(ImDrawVert),
                static_cast<uint32_t>(p_draw_data->TotalIdxCount));

            ImDrawVert* p_vertex_buffer = static_cast<ImDrawVert*>(m_vertex_and_index_data_allocation->cpuAddress());
            ImDrawIdx* p_index_buffer = reinterpret_cast<ImDrawIdx*>(static_cast<unsigned char*>(m_vertex_and_index_data_allocation->cpuAddress()) + index_buffer_offset);
            for (ImDrawList const* p_draw_list : p_draw_data->CmdLists)
            {
                memcpy(p_vertex_buffer, p_draw_list->VtxBuffer.Data, p_draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
                memcpy(p_index_buffer, p_draw_list->IdxBuffer.Data, p_draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));
                p_vertex_buffer += p_draw_list->VtxBuffer.Size;
                p_index_buffer += p_draw_list->IdxBuffer.Size;
            }
        }

        // Render the UI
        {
            setup_render_state_lambda();

            int offset_in_vertex_buffer{ 0 }, offset_in_index_buffer{ 0 };
            ImVec2 clip_off = p_draw_data->DisplayPos;
            ImVec2 clip_scale = p_draw_data->FramebufferScale;
            for (ImDrawList const* p_draw_list : p_draw_data->CmdLists)
            {
                for (int cmd_idx = 0; cmd_idx < p_draw_list->CmdBuffer.Size; ++cmd_idx)
                {
                    ImDrawCmd const* p_draw_command = &p_draw_list->CmdBuffer[cmd_idx];
                    if (p_draw_command->UserCallback != nullptr)
                    {
                        if (p_draw_command->UserCallback == ImDrawCallback_ResetRenderState)
                            setup_render_state_lambda();
                        else
                            p_draw_command->UserCallback(p_draw_list, p_draw_command);
                    }
                    else
                    {
                        math::Vector2f clip_min{
                            (p_draw_command->ClipRect.x - clip_off.x) * clip_scale.x,
                            (p_draw_command->ClipRect.y - clip_off.y) * clip_scale.y
                        };
						math::Vector2f clip_max{
							(p_draw_command->ClipRect.z - clip_off.x) * clip_scale.x,
							(p_draw_command->ClipRect.w - clip_off.y) * clip_scale.y
						};
                        if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                        {
                            continue;
                        }

                        math::Rectangle& scissor_rectangle = m_scissor_rectangles[0];
                        scissor_rectangle.setUpperLeft(clip_min);

                        math::Vector2f scissor_rectangle_size = clip_max - clip_min;
                        scissor_rectangle.setSize(scissor_rectangle_size.x, scissor_rectangle_size.y);
                        m_cmd_list_ptr->rasterizerStateSetScissorRectangles(m_scissor_rectangles);
						auto* p_shader_stage = m_shader_function.getShaderStage(dxcompilation::ShaderType::pixel);
                        ImTextureID textureId = p_draw_command->GetTexID();
						if (textureId != m_currentlyBoundImGuiTextureId)
                        {
							p_shader_stage->bindTexture("texture0", *m_imgui_textures[textureId]);
                            m_currentlyBoundImGuiTextureId = textureId;
						}
						m_cmd_list_ptr->drawIndexedInstanced(
							p_draw_command->ElemCount,
							1,
							p_draw_command->IdxOffset + offset_in_index_buffer,
							p_draw_command->VtxOffset + offset_in_vertex_buffer,
							0
						);
                    }
                }
                offset_in_index_buffer += p_draw_list->IdxBuffer.Size;
                offset_in_vertex_buffer += p_draw_list->VtxBuffer.Size;
            }
        }

        m_basic_rendering_services.endRendering(*m_cmd_list_ptr);
    }
}
