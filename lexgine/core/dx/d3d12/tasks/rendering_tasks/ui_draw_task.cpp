#include <chrono>

#include "lexgine/core/exception.h"
#include "lexgine/core/globals.h"
#include "lexgine/core/ui.h"
#include "lexgine/core/dx/d3d12/dx_resource_factory.h"
#include "lexgine/core/dx/d3d12/device.h"
#include "lexgine/core/dx/d3d12/resource.h"
#include "lexgine/core/dx/d3d12/resource_barrier_pack.h"
#include "lexgine/core/dx/d3d12/basic_rendering_services.h"
#include "lexgine/core/dx/d3d12/descriptor_table_builders.h"
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

std::shared_ptr<UIDrawTask> UIDrawTask::create(Globals& globals, BasicRenderingServices& basic_rendering_services)
{
    std::shared_ptr<UIDrawTask> rv{ new UIDrawTask{globals, basic_rendering_services} };
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
        m_rendering_window_ptr = rendering_configuration.p_rendering_window;
        m_rendering_window_ptr->addListener(shared_from_this());
        defineImGUIKeyMap(m_rendering_window_ptr->native());
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
            m_pso->setRootSignatureCompilationTask(m_rs);
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
    return mouseButtonHandler(button, xbutton_id, *m_rendering_window_ptr, true);
}

bool UIDrawTask::buttonUp(MouseButton button, uint16_t xbutton_id, osinteraction::windows::ControlKeyFlag const& control_key_flag, uint16_t x, uint16_t y)
{
    return mouseButtonHandler(button, xbutton_id, *m_rendering_window_ptr, false);
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

bool UIDrawTask::setCursor()
{
    return updateMouseCursor();
}


enum class Test
{
    a0, a1, a2
};

UIDrawTask::UIDrawTask(Globals& globals, BasicRenderingServices& basic_rendering_services)
    : GpuWorkSource{ *globals.get<Device>(), CommandType::direct }
    , m_globals{ globals }
    , m_device{ *globals.get<Device>() }
    , m_basic_rendering_services{ basic_rendering_services }
    , m_time_counter{ std::chrono::high_resolution_clock::now().time_since_epoch().count() }
    , m_resource_uploader{ globals, basic_rendering_services.resourceUploadAllocator() }
    , m_projection_matrix_constants(16)
    , m_scissor_rectangles(1)
{
    m_viewports.emplace_back(math::Vector2f{ 0.f }, math::Vector2f{ 0.f }, math::Vector2f{ 0.f });

    IMGUI_CHECKVERSION();
    m_gui_context = ImGui::CreateContext();
    ImGui::StyleColorsDark();

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
                RootEntryDescriptorTable font_texture_srv_table{};
                font_texture_srv_table.addRange(RootEntryDescriptorTable::RangeType::srv, 1, 0, 0, 0);
                rs.addParameter(1, font_texture_srv_table, ShaderVisibility::pixel);
            }

            // static bilinear sampler
            {
                FilterPack filtering_parameters{ MinificationFilter::linear_mipmap_linear, MagnificationFilter::linear, 0,
                WrapMode::repeat, WrapMode::repeat, WrapMode::repeat, StaticBorderColor::transparent_black };
                RootStaticSampler sampler{ 0, 0, filtering_parameters };
                rs.addStaticSampler(sampler, ShaderVisibility::pixel);
            }

            auto flags = RootSignatureFlags::base_values::allow_input_assembler 
                | RootSignatureFlags::base_values::deny_hull_shader
                | RootSignatureFlags::base_values::deny_domain_shader
                | RootSignatureFlags::base_values::deny_geometry_shader;

            m_rs = rs_compilation_task_cache.findOrCreateTask(globals, std::move(rs), flags, "ui_rendering_rs", 0);
            m_rs->execute(0);
        }

        // shaders
        {
            static char const* const hlsl_source =
                "struct ConstantBufferDataStruct\n\
                {\n\
                    float4x4 ProjectionMatrix;\n\
                };\n \
                ConstantBuffer<ConstantBufferDataStruct> constants : register(b0);\n\
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
        }
    }

    // Fetch and upload UI font texture
    {
        unsigned char* pixels{ nullptr };
        int width{}, height{};
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        ResourceDescriptor font_texture_descriptor = ResourceDescriptor::CreateTexture2D(width, height, 1, DXGI_FORMAT_R8G8B8A8_UNORM);

        m_fonts_texture = std::make_unique<CommittedResource>(m_device, ResourceState::base_values::pixel_shader,
            misc::makeEmptyOptional<ResourceOptimizedClearValue>(), font_texture_descriptor, AbstractHeapType::default,
            HeapCreationFlags::base_values::allow_all);

        ResourceDataUploader::TextureSourceDescriptor source_descriptor{};
        ResourceDataUploader::TextureSourceDescriptor::Subresource texture_subresource{
            pixels,
            static_cast<size_t>(width) * 4ULL,
            static_cast<size_t>(width) * static_cast<size_t>(height) * 4ULL
        };
        source_descriptor.subresources.emplace_back(texture_subresource);

        ResourceDataUploader::DestinationDescriptor destination_descriptor{};
        destination_descriptor.p_destination_resource = m_fonts_texture.get();
        destination_descriptor.destination_resource_state = ResourceState::base_values::pixel_shader;
        destination_descriptor.segment.subresources.first_subresource = 0;
        destination_descriptor.segment.subresources.num_subresources = 1;

        m_resource_uploader.addResourceForUpload(destination_descriptor, source_descriptor);
        m_resource_uploader.upload();
        m_resource_uploader.waitUntilUploadIsFinished();

        {
            ResourceViewDescriptorTableBuilder builder{ globals, 0 };
            builder.addDescriptor(SRVDescriptor{ *m_fonts_texture, SRVTextureInfo{} });

            m_srv_table = builder.build();
            io.Fonts->TexID = reinterpret_cast<ImTextureID>(m_srv_table.gpu_pointer);
        }
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
        m_ui_index_data_binding = std::make_unique<IndexBufferBinding>(m_ui_data_allocator->getUploadResource(), 0, index_data_type, 0);
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
            std::make_shared<VertexAttributeSpecification<uint8_t, 4>>(0, static_cast<unsigned char>(IM_OFFSETOF(ImDrawVert, col)), "COLOR", 0, 0)
            );
        m_va_list = VertexAttributeSpecificationList{ position, uv, color };
    }
}

bool UIDrawTask::doTask(uint8_t worker_id, uint64_t user_data)
{
    if (!m_rendering_window_ptr) return true;

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
    GetClientRect(m_rendering_window_ptr->native(), &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    long long current_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    io.DeltaTime = static_cast<float>(current_time - m_time_counter) 
        * std::chrono::nanoseconds::period::num / std::chrono::nanoseconds::period::den;
    m_time_counter = current_time;

    // Read keyboard modifiers inputs
    io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    io.KeySuper = false;
    // io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.

    // Update OS mouse position
    updateMousePosition(m_rendering_window_ptr->native());

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

void UIDrawTask::drawFrame()
{
    if(ImDrawData* p_draw_data = ImGui::GetDrawData())
    {
        auto& cmd_list = gpuWorkPackage();
        cmd_list.reset();
        m_basic_rendering_services.setDefaultResources(cmd_list);
        // m_basic_rendering_services.beginRendering(m_cmd_list);

        auto setup_render_state_lambda = [this, p_draw_data, &cmd_list]()
        {
            Viewport viewport{ 
                math::Vector2f{0.f},
                math::Vector2f{p_draw_data->DisplaySize.x, p_draw_data->DisplaySize.y},
                math::Vector2f{0.f, 1.f} 
            };
            m_viewports[0] = viewport;

            auto projection_matrix = math::createOrthogonalProjectionMatrix(misc::EngineAPI::Direct3D12,
                p_draw_data->DisplayPos.x, p_draw_data->DisplayPos.y, 
                p_draw_data->DisplaySize.x, p_draw_data->DisplaySize.y, -1.f, 1.f);

            std::transform(projection_matrix.getRawData(), projection_matrix.getRawData() + 16,
                m_projection_matrix_constants.begin(), [](float value) {return *reinterpret_cast<uint32_t*>(&value); });

            cmd_list.setPipelineState(m_pso->getTaskData());
            cmd_list.setRootSignature(m_rs->getCacheName());
            cmd_list.setRoot32BitConstants(0, m_projection_matrix_constants, 0);
            cmd_list.rasterizerStateSetViewports(m_viewports);
            cmd_list.inputAssemblySetVertexBuffers(*m_ui_vertex_data_binding);
            cmd_list.inputAssemblySetIndexBuffer(*m_ui_index_data_binding);
            cmd_list.inputAssemblySetPrimitiveTopology(PrimitiveTopology::triangle_list);
            cmd_list.outputMergerSetBlendFactor(math::Vector4f{ 0.f });
            m_basic_rendering_services.setDefaultRenderingTarget(cmd_list);
        };

        // upload vertex data and set rendering context state
        {
            if (p_draw_data->DisplaySize.x <= 0 || p_draw_data->DisplaySize.y <= 0)
                return;

            size_t total_vertex_count = p_draw_data->TotalVtxCount + 5000;
            size_t total_index_count = p_draw_data->TotalIdxCount + 10000;
            size_t index_buffer_offset = total_vertex_count * sizeof(ImDrawVert);
            size_t required_draw_buffer_capacity = index_buffer_offset + total_index_count * sizeof(ImDrawIdx);
            
            m_vertex_and_index_data_allocation = m_ui_data_allocator->allocate(required_draw_buffer_capacity);
            m_ui_vertex_data_binding->setVertexBufferView(0, m_ui_data_allocator->getUploadResource(), m_vertex_and_index_data_allocation->offset(),
                sizeof(ImDrawVert), static_cast<uint32_t>(p_draw_data->TotalVtxCount));
            m_ui_index_data_binding->update(m_vertex_and_index_data_allocation->offset() + total_vertex_count * sizeof(ImDrawVert),
                static_cast<uint32_t>(p_draw_data->TotalIdxCount));

            ImDrawVert* p_vertex_buffer = static_cast<ImDrawVert*>(m_vertex_and_index_data_allocation->cpuAddress());
            ImDrawIdx* p_index_buffer = reinterpret_cast<ImDrawIdx*>(static_cast<unsigned char*>(m_vertex_and_index_data_allocation->cpuAddress()) + index_buffer_offset);
            for (int i = 0; i < p_draw_data->CmdListsCount; ++i)
            {
                ImDrawList const* p_draw_list = p_draw_data->CmdLists[i];
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
            for (int i = 0; i < p_draw_data->CmdListsCount; ++i)
            {
                ImDrawList const* p_draw_list = p_draw_data->CmdLists[i];
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
                        math::Rectangle& scissor_rectangle = m_scissor_rectangles[0];
                        scissor_rectangle.setUpperLeft(math::Vector2f{ p_draw_command->ClipRect.x - clip_off.x,
                            p_draw_command->ClipRect.y - clip_off.y });
                        scissor_rectangle.setSize(p_draw_command->ClipRect.z - p_draw_command->ClipRect.x,
                            p_draw_command->ClipRect.w - p_draw_command->ClipRect.y);
                        ShaderResourceDescriptorTable srv_table{};
                        srv_table.gpu_pointer = reinterpret_cast<uint64_t>(p_draw_command->TextureId);
                        cmd_list.setRootDescriptorTable(1, srv_table);
                        cmd_list.rasterizerStateSetScissorRectangles(m_scissor_rectangles);
                        cmd_list.drawIndexedInstanced(p_draw_command->ElemCount, 1, offset_in_index_buffer, offset_in_vertex_buffer, 0);
                    }

                    offset_in_index_buffer += p_draw_command->ElemCount;
                }
                offset_in_vertex_buffer += p_draw_list->VtxBuffer.Size;
            }
        }

        m_basic_rendering_services.endRendering(cmd_list);
        cmd_list.close();
    }
}
