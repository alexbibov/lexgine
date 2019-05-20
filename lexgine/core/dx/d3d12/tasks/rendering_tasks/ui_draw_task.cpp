#include <chrono>

#include "lexgine/core/exception.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/dx/d3d12/dx_resource_factory.h"
#include "lexgine/core/dx/d3d12/device.h"
#include "lexgine/core/dx/d3d12/resource.h"
#include "lexgine/core/dx/d3d12/basic_rendering_services.h"
#include "lexgine/core/dx/d3d12/srv_descriptor.h"
#include "lexgine/core/dx/d3d12/tasks/root_signature_compilation_task.h"
#include "lexgine/core/dx/d3d12/tasks/pso_compilation_task.h"
#include "lexgine/core/dx/d3d12/tasks/hlsl_compilation_task.h"
#include "lexgine/core/math/utility.h"
#include "ui_draw_task.h"


using namespace lexgine;
using namespace lexgine::core;
using namespace lexgine::core::dx::d3d12;
using namespace lexgine::core::dx::d3d12::tasks::rendering_tasks;

std::string const UIDrawTask::c_interface_update_section = "interface_update_section";

namespace {

void defineImGUIKeyMap(HWND hwnd)
{
    ImGuiIO& io = ImGui::GetIO();

    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName = "lexgine_profiler";
    io.ImeWindowHandle = hwnd;

    io.KeyMap[ImGuiKey_Tab] = static_cast<int>(osinteraction::SystemKey::tab);
    io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int>(osinteraction::SystemKey::arrow_left);
    io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(osinteraction::SystemKey::arrow_right);
    io.KeyMap[ImGuiKey_UpArrow] = static_cast<int>(osinteraction::SystemKey::arrow_up);
    io.KeyMap[ImGuiKey_DownArrow] = static_cast<int>(osinteraction::SystemKey::arrow_down);
    io.KeyMap[ImGuiKey_PageUp] = static_cast<int>(osinteraction::SystemKey::page_up);
    io.KeyMap[ImGuiKey_PageDown] = static_cast<int>(osinteraction::SystemKey::page_down);
    io.KeyMap[ImGuiKey_Home] = static_cast<int>(osinteraction::SystemKey::home);
    io.KeyMap[ImGuiKey_End] = static_cast<int>(osinteraction::SystemKey::end);
    io.KeyMap[ImGuiKey_Insert] = static_cast<int>(osinteraction::SystemKey::insert);
    io.KeyMap[ImGuiKey_Delete] = static_cast<int>(osinteraction::SystemKey::_delete);
    io.KeyMap[ImGuiKey_Backspace] = static_cast<int>(osinteraction::SystemKey::backspace);
    io.KeyMap[ImGuiKey_Space] = static_cast<int>(osinteraction::SystemKey::space);
    io.KeyMap[ImGuiKey_Enter] = static_cast<int>(osinteraction::SystemKey::enter);
    io.KeyMap[ImGuiKey_Escape] = static_cast<int>(osinteraction::SystemKey::esc);
    io.KeyMap[ImGuiKey_A] = static_cast<int>(osinteraction::SystemKey::A);
    io.KeyMap[ImGuiKey_C] = static_cast<int>(osinteraction::SystemKey::C);
    io.KeyMap[ImGuiKey_V] = static_cast<int>(osinteraction::SystemKey::V);
    io.KeyMap[ImGuiKey_X] = static_cast<int>(osinteraction::SystemKey::X);
    io.KeyMap[ImGuiKey_Y] = static_cast<int>(osinteraction::SystemKey::Y);
    io.KeyMap[ImGuiKey_Z] = static_cast<int>(osinteraction::SystemKey::Z);
}

bool mouseButtonHandler(osinteraction::windows::MouseButtonListener::MouseButton button, uint16_t xbutton_id, 
    osinteraction::windows::Window const& window, bool acquire_capture)
{
    ImGuiIO& io = ImGui::GetIO();

    switch (button)
    {
    case osinteraction::windows::MouseButtonListener::MouseButton::left:
        io.MouseDown[0] = acquire_capture;
        break;
    case osinteraction::windows::MouseButtonListener::MouseButton::middle:
        io.MouseDown[2] = acquire_capture;
        break;
    case osinteraction::windows::MouseButtonListener::MouseButton::right:
        io.MouseDown[1] = acquire_capture;
        break;
    case osinteraction::windows::MouseButtonListener::MouseButton::x:
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

bool updateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
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
        }
        SetCursor(LoadCursor(NULL, win32_cursor));
    }
    return true;
}

void updateMousePosition(HWND hwnd)
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantSetMousePos)
    {
        POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
        ClientToScreen(hwnd, &pos);
        SetCursorPos(pos.x, pos.y);
    }

    // Set mouse position
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    POINT pos;
    if (HWND active_window = GetForegroundWindow())
    {
        if (active_window == hwnd || IsChild(active_window, hwnd))
            if (GetCursorPos(&pos) && ScreenToClient(hwnd, &pos))
                io.MousePos = ImVec2((float)pos.x, (float)pos.y);
    }
}

}


UIDrawTask::UIDrawTask(Globals& globals, BasicRenderingServices& basic_rendering_services,
    osinteraction::windows::Window& rendering_window)
    : m_globals{ globals }
    , m_device{ *globals.get<Device>() }
    , m_basic_rendering_services{ basic_rendering_services }
    , m_rendering_window{ rendering_window }
    , m_time_counter{ std::chrono::high_resolution_clock::now().time_since_epoch().count() }
    , m_resource_uploader{ globals, basic_rendering_services.resourceUploadAllocator() }
    , m_cb_data_mapping{ m_cb_reflection }
{
    m_rendering_window.addListener(this);

    defineImGUIKeyMap(m_rendering_window.native());

    ImGuiIO& io = ImGui::GetIO();
    m_mouse_cursor = io.MouseDrawCursor
        ? ImGuiMouseCursor_None
        : ImGui::GetMouseCursor();

    // Setup UI device objects
    {
        // root signature
        {
            task_caches::RootSignatureCompilationTaskCache& rs_compilation_task_cache = *m_globals.get<task_caches::RootSignatureCompilationTaskCache>();
            RootSignature rs{};

            // parameter 0 (rendering constants, root constants)
            {
                RootEntryConstants root_constants{ 0, 0, 16 };
                rs.addParameter(0, root_constants, ShaderVisibility::vertex);
            }

            // parameter 1 (font texture, root SRV)
            {
                RootEntrySRVDescriptor root_srv_descriptor{ 0, 0 };
                rs.addParameter(1, root_srv_descriptor, ShaderVisibility::pixel);
            }

            // static bilinear sampler
            {
                FilterPack filtering_parameters{ MinificationFilter::linear_mipmap_linear, MagnificationFilter::linear, 0,
                WrapMode::repeat, WrapMode::repeat, WrapMode::repeat, StaticBorderColor::transparent_black };
                RootStaticSampler sampler{ 0, 0, filtering_parameters };
                rs.addStaticSampler(sampler, ShaderVisibility::pixel);
            }

            RootSignatureFlags flags = RootSignatureFlags::enum_type::allow_input_assembler;
            flags |= RootSignatureFlags::enum_type::deny_hull_shader;
            flags |= RootSignatureFlags::enum_type::deny_domain_shader;
            flags |= RootSignatureFlags::enum_type::deny_geometry_shader;

            m_rs = rs_compilation_task_cache.findOrCreateTask(globals, std::move(rs), flags, "ui_rendering_rs", 0);
        }

        // shaders
        {
            static char const* const hlsl_source =
                "struct ConstantBufferDataStruct\
                {\
                    float4x4 ProjectionMatrix; \
                }; \
                ConstantBuffer<ConstantBufferDataStruct> vertexBuffer : register(b0) \
                {\
                    float4x4 ProjectionMatrix; \
                }; \
                struct VS_INPUT\
                {\
                    float2 pos : POSITION; \
                    float4 col : COLOR0; \
                    float2 uv : TEXCOORD0; \
                }; \
                \
                struct PS_INPUT\
                {\
                    float4 pos : SV_POSITION; \
                    float4 col : COLOR0; \
                    float2 uv : TEXCOORD0; \
                }; \
                \
                PS_INPUT VSMain(VS_INPUT input)\
                {\
                    PS_INPUT output; \
                    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f)); \
                    output.col = input.col; \
                    output.uv = input.uv; \
                    return output; \
                }\
                \
                SamplerState sampler0 : register(s0);\
                Texture2D<float4> texture0 : register(t0);\
                \
                float4 PSMain(PS_INPUT input) : SV_Target\
                {\
                    float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
                    return out_col; \
                }";

            task_caches::HLSLCompilationTaskCache& hlsl_compilation_task_cache = *m_globals.get<task_caches::HLSLCompilationTaskCache>();
            task_caches::HLSLSourceTranslationUnit hlsl_translation_unit{ m_globals, "ui_rendering_shader", hlsl_source };

            m_vs = hlsl_compilation_task_cache.findOrCreateTask(hlsl_translation_unit, dxcompilation::ShaderModel::model_62, dxcompilation::ShaderType::vertex, "VSMain");
            m_ps = hlsl_compilation_task_cache.findOrCreateTask(hlsl_translation_unit, dxcompilation::ShaderModel::model_62, dxcompilation::ShaderType::pixel, "PSMain");
        }
    }

    // Fetch and upload UI font texture
    {
        unsigned char* pixels{ nullptr };
        int width{}, height{};
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        ResourceDescriptor font_texture_descriptor = ResourceDescriptor::CreateTexture2D(width, height, 1, DXGI_FORMAT_R8G8B8A8_UNORM);

        m_fonts_texture = std::make_unique<CommittedResource>(m_device, ResourceState::enum_type::pixel_shader,
            misc::makeEmptyOptional<ResourceOptimizedClearValue>(), font_texture_descriptor, AbstractHeapType::default,
            HeapCreationFlags::enum_type::allow_all);

        ResourceDataUploader::TextureSourceDescriptor source_descriptor{};
        ResourceDataUploader::TextureSourceDescriptor::Subresource texture_subresource{ 
            pixels, 
            static_cast<size_t>(width) * 4ULL, 
            static_cast<size_t>(width) * static_cast<size_t>(height) * 4ULL 
        };
        source_descriptor.subresources.emplace_back(texture_subresource);

        ResourceDataUploader::DestinationDescriptor destination_descriptor{};
        destination_descriptor.p_destination_resource = m_fonts_texture.get();
        destination_descriptor.destination_resource_state = ResourceState::enum_type::pixel_shader;
        destination_descriptor.segment.subresources.first_subresource = 0;
        destination_descriptor.segment.subresources.num_subresources = 1;

        m_resource_uploader.addResourceForUpload(destination_descriptor, source_descriptor);
        m_resource_uploader.upload();
        m_resource_uploader.waitUntilUploadIsFinished();

        io.Fonts->TexID = reinterpret_cast<ImTextureID>(m_fonts_texture->getGPUVirtualAddress());
    }

    // Create UI update section in upload heap
    {
        DxResourceFactory& dx_resource_factory = *globals.get<DxResourceFactory>();
        Device& device = *globals.get<Device>();

        auto ui_update_section = dx_resource_factory.allocateSectionInUploadHeap(dx_resource_factory.retrieveUploadHeap(device),
            UIDrawTask::c_interface_update_section,
            UIDrawTask::c_interface_update_section_size);

        if (!ui_update_section.isValid())
        {
            LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
                "Unable to allocate UI update section \"" + UIDrawTask::c_interface_update_section
                + "\" in data upload heap");
        }

        UploadHeapPartition const& ui_update_section_partition = static_cast<UploadHeapPartition const&>(ui_update_section);
        m_ui_data_allocator = std::make_unique<PerFrameUploadDataStreamAllocator>(globals, ui_update_section_partition.offset, ui_update_section_partition.size,
            dx_resource_factory.retrieveFrameProgressTracker(device));
    }


    // Setup constant buffer data
    {
        auto const& viewport = m_basic_rendering_services.defaultViewport();
        auto viewport_top_left_corner = viewport.topLeftCorner();

        m_projection_transform = math::createOrthogonalProjectionMatrix(misc::EngineAPI::Direct3D12,
            viewport_top_left_corner.x, viewport_top_left_corner.y, viewport.width(), viewport.height(), -1.f, 1.f);

        m_cb_reflection.addElement("ProjectionMatrix",
            ConstantBufferReflection::ReflectionEntryDesc{ ConstantBufferReflection::ReflectionEntryBaseType::float4x4, 1 });

        m_cb_data_mapping.addDataBinding("ProjectionMatrix", m_projection_transform);
    }

    // Create input layout
    {
        std::shared_ptr<AbstractVertexAttributeSpecification> position = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
            std::make_shared<VertexAttributeSpecification<float, 2>>(0, static_cast<unsigned char>(IM_OFFSETOF(ImDrawVert, pos)), "POSITION", 0, 0)
            );
        std::shared_ptr<AbstractVertexAttributeSpecification> uv = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
            std::make_shared<VertexAttributeSpecification<float, 2>>(0, static_cast<unsigned char>(IM_OFFSETOF(ImDrawVert, uv)), "TEXCOORD", 0, 0)
            );
        std::shared_ptr<AbstractVertexAttributeSpecification> color = std::static_pointer_cast<AbstractVertexAttributeSpecification>(
            std::make_shared<VertexAttributeSpecification<float, 4>>(0, static_cast<unsigned char>(IM_OFFSETOF(ImDrawVert, col)), "COLOR", 0, 0)
            );
        m_va_list = VertexAttributeSpecificationList{ position, uv, color };
    }
}

UIDrawTask::~UIDrawTask() = default;

void UIDrawTask::updateBufferFormats(DXGI_FORMAT color_buffer_format, DXGI_FORMAT depth_buffer_format)
{
    task_caches::PSOCompilationTaskCache& pso_compilation_task_cache = *m_globals.get<task_caches::PSOCompilationTaskCache>();

    if(!m_pso)
    {
        m_pso_desc.node_mask = 0x1;
        m_pso_desc.vertex_attributes = m_va_list;
        m_pso_desc.primitive_topology_type = PrimitiveTopologyType::triangle;
        m_pso_desc.num_render_targets = 1;
        m_pso_desc.rtv_formats[0] = color_buffer_format;
        m_pso_desc.dsv_format = depth_buffer_format;

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
            "ui_rendering_pso__" + std::to_string(color_buffer_format) + "__" + std::to_string(depth_buffer_format), 0);
        m_pso->setVertexShaderCompilationTask(m_vs);
        m_pso->setPixelShaderCompilationTask(m_ps);
        m_pso->setRootSignatureCompilationTask(m_rs);
    }
    else
    {
        m_pso_desc.rtv_formats[0] = color_buffer_format;
        m_pso_desc.dsv_format = depth_buffer_format;
        m_pso = pso_compilation_task_cache.findOrCreateTask(m_globals, m_pso_desc,
            "ui_rendering_pso__" + std::to_string(color_buffer_format) + "__" + std::to_string(depth_buffer_format), 0);
    }
}

bool UIDrawTask::keyDown(osinteraction::SystemKey key)
{
    return true;
}

bool UIDrawTask::keyUp(osinteraction::SystemKey key)
{
    return true;
}

bool UIDrawTask::character(wchar_t char_key)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(char_key);
    return true;
}

bool UIDrawTask::systemKeyDown(osinteraction::SystemKey key)
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[static_cast<size_t>(key)] = 1;
    return true;
}

bool UIDrawTask::systemKeyUp(osinteraction::SystemKey key)
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[static_cast<size_t>(key)] = 0;
    return true;
}


bool UIDrawTask::buttonDown(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    return mouseButtonHandler(button, xbutton_id, m_rendering_window, true);
}

bool UIDrawTask::buttonUp(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    return mouseButtonHandler(button, xbutton_id, m_rendering_window, false);
}

bool UIDrawTask::doubleClick(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    return buttonDown(button, xbutton_id, control_key_flag, x, y);
}

bool UIDrawTask::wheelMove(double move_delta, bool is_horizontal_wheel, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
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


bool UIDrawTask::move(uint16_t x, uint16_t y, osinteraction::windows::ControlKeyFlag const& control_key_flag)
{
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

bool UIDrawTask::setCursor()
{
    return updateMouseCursor();
}

bool UIDrawTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    processEvents();


    return true;
}

void UIDrawTask::processEvents() const
{
    ImGuiIO& io = ImGui::GetIO();

    if (!io.Fonts->IsBuilt())
    {
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this,
            "Font atlas not built! It is generally built by the renderer back-end. "
            "Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");
    }

    // Setup display size (every frame to accommodate for window resizing)
    RECT rect;
    GetClientRect(m_rendering_window.native(), &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    long long current_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    io.DeltaTime = static_cast<float>(current_time - m_time_counter) / c_update_ticks_per_second;
    m_time_counter = current_time;

    // Read keyboard modifiers inputs
    io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    io.KeySuper = false;
    // io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.

    // Update OS mouse position
    updateMousePosition(m_rendering_window.native());

    // Update OS mouse cursor with the cursor requested by imgui
    ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor 
        ? ImGuiMouseCursor_None 
        : ImGui::GetMouseCursor();
    if (m_mouse_cursor != mouse_cursor)
    {
        m_mouse_cursor = mouse_cursor;
        updateMouseCursor();
    }

    // TODO: support gamepads
}
