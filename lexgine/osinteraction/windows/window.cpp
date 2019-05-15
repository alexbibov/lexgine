#include <algorithm>
#include <map>

#define OEMRESOURCE

#include "lexgine/core/exception.h"
#include "window.h"

using namespace lexgine;
using namespace lexgine::osinteraction;
using namespace lexgine::osinteraction::windows;


ATOM Window::m_atom = 0;
uint32_t Window::m_window_counter = 0;


namespace
{
static wchar_t const window_class_name[] = L"LexgineWindow"; //!< string name of Lexgine window class
static std::map<HWND, Window*> window_pool{};    //!< pool of windows managed by this window callback procedure

//! Helper. Returns the last error recorded by the operating system for this app
std::string getLastSystemError()
{
    LPWSTR lpErrMsgBuffer;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(0x09, 0x01),
        reinterpret_cast<LPWSTR>(&lpErrMsgBuffer), 1024, 0);
    size_t err_msg_buf_len = wcslen(lpErrMsgBuffer);
    char* err_msg_chars = new char[err_msg_buf_len + 1];
    for (size_t i = 0; i < err_msg_buf_len; ++i)
    {
        err_msg_chars[i] = static_cast<char>(lpErrMsgBuffer[i]);
    }
    HeapFree(GetProcessHeap(), 0, lpErrMsgBuffer);

    err_msg_chars[err_msg_buf_len] = 0;
    std::string rv{ err_msg_chars };
    delete[] err_msg_chars;

    return rv;
}


HWND createWindow(Window* p_window, ATOM& atom, HINSTANCE hInstance, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
    Window::WindowStyle const& window_style, Window::WindowExStyle const& window_extended_style, std::wstring const& title, WNDPROC window_procedure)
{
    // Check consistency of the window styles
    Window::WindowStyle::base_int_type window_style_flags;
    Window::WindowExStyle::base_int_type window_ex_style_flags = static_cast<Window::WindowExStyle::base_int_type>(window_extended_style);

    if (window_style.isSet(Window::WindowStyle::enum_type::HasMaximizeButton) ||
        window_style.isSet(Window::WindowStyle::enum_type::HasMinimizeButton))
        window_style_flags = static_cast<Window::WindowStyle::base_int_type>(window_style | Window::WindowStyle::enum_type::HasSystemMenu);

    if (window_style.isSet(Window::WindowStyle::enum_type::HasSystemMenu))
        window_style_flags |= static_cast<Window::WindowStyle::base_int_type>(Window::WindowStyle::enum_type::HasTitleBar);


    // Create window class if it has not been provided by the caller
    if(!atom)
    {
        WNDCLASSEX wndclassex;
        wndclassex.cbSize = sizeof(WNDCLASSEX);
        wndclassex.style = CS_DBLCLKS | CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
        wndclassex.lpfnWndProc = window_procedure;
        wndclassex.cbClsExtra = 0;
        wndclassex.cbWndExtra = 0;
        wndclassex.hInstance = hInstance;
        wndclassex.hIcon = NULL;	//This should be changed to the TinyWorld icon in future!!!
        wndclassex.hCursor = static_cast<HCURSOR>(LoadImage(NULL, MAKEINTRESOURCE(OCR_NORMAL), IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
        wndclassex.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
        wndclassex.lpszMenuName = NULL;
        wndclassex.lpszClassName = const_cast<wchar_t*>(window_class_name);
        wndclassex.hIconSm = NULL;	//This should be changed to the TinyWorld icon in future!!!

        if (!(atom = RegisterClassEx(&wndclassex))) return NULL;
    }

    return CreateWindowEx(window_ex_style_flags, const_cast<wchar_t*>(window_class_name), title.c_str(), window_style_flags, x, y, width, height, NULL, NULL, hInstance,
        static_cast<LPVOID>(p_window));
}


}




Window::Window(HINSTANCE hInstance /* = NULL */,
    WindowStyle window_style /* = WindowStyle
                             | WindowStyle::enum_type::HasSystemMenu
                             | WindowStyle::enum_type::HasMinimizeButton
                             | WindowStyle::enum_type::HasMaximizeButton
                             | WindowStyle::enum_type::SupportsSizing */,
    WindowExStyle window_ex_style /* = WindowExStyle::enum_type::RaisedBorderEdge */) :
    m_hinstance{ hInstance ? hInstance : GetModuleHandle(NULL) },
    m_hwnd{ NULL },
    m_window_style{ window_style },
    m_window_ex_style{ window_ex_style },
    m_title(L"LexgineDXWindow"),
    m_pos_x{ 0 },
    m_pos_y{ 0 },
    m_width{ 640 },
    m_height{ 480 },
    m_is_visible{ false },
    m_should_close{ false }
{
    adjustClientAreaSize();

    if (!(m_hwnd = createWindow(this, m_atom, m_hinstance, m_pos_x, m_pos_y, m_width, m_height, m_window_style, m_window_ex_style, m_title, WindowProcedure)))
    {
        std::string error_string = getLastSystemError();
        logger().out(error_string, core::misc::LogMessageType::error);
        raiseError(error_string);
        throw;
    }
    else
    {
        ++m_window_counter;
    }
}

Window::~Window()
{
    if (m_hwnd)
        DestroyWindow(m_hwnd);

    if (!m_window_counter && m_atom)
    {
        UnregisterClass(const_cast<wchar_t*>(window_class_name), m_hinstance);
        m_atom = 0;
    }
}

void Window::setTitle(std::wstring const& title)
{
    if (!m_hwnd) return;

    m_title = title;
    if (!SetWindowText(m_hwnd, title.c_str()))
    {
        std::string error_string = getLastSystemError();
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, error_string);
    }
}


std::wstring Window::getTitle() const { return m_title; }


void Window::setDimensions(uint32_t width, uint32_t height)
{
    if (!m_hwnd) return;

    m_width = width;
    m_height = height;

    adjustClientAreaSize();

    WINDOWPLACEMENT wndplacement;
    wndplacement.length = sizeof(WINDOWPLACEMENT);
    wndplacement.flags = WPF_ASYNCWINDOWPLACEMENT;
    wndplacement.showCmd = m_is_visible ? SW_SHOW : SW_HIDE;
    wndplacement.ptMaxPosition = POINT{ 0, 0 };
    wndplacement.rcNormalPosition = RECT{ static_cast<LONG>(m_pos_x), static_cast<LONG>(m_pos_y),
        static_cast<LONG>(m_pos_x + width), static_cast<LONG>(m_pos_y + height) };

    if (!SetWindowPlacement(m_hwnd, &wndplacement))
    {
        std::string error_string = getLastSystemError();
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, error_string);
    }
}


void Window::setDimensions(core::math::Vector2u const& dimensions)
{
    setDimensions(dimensions.x, dimensions.y);
}


lexgine::core::math::Vector2u Window::getDimensions() const{ return core::math::Vector2u{ m_width, m_height }; }


void Window::setLocation(uint32_t x, uint32_t y)
{
    if (!m_hwnd) return;

    m_pos_x = x; m_pos_y = y;
    setDimensions(m_width, m_height);
}


void Window::setLocation(core::math::Vector2u const& location)
{
    setLocation(location.x, location.y);
}


lexgine::core::math::Vector2u Window::getLocation() const { return core::math::Vector2u{ m_pos_x, m_pos_y }; }


void Window::setVisibility(bool visibility_flag)
{
    if (!m_hwnd) return;

    m_is_visible = visibility_flag;
    ShowWindow(m_hwnd, visibility_flag ? SW_SHOW : SW_HIDE);
}


bool Window::getVisibility() const
{
    return m_is_visible;
}


bool Window::shouldClose() const { return m_should_close; }


HWND Window::native() const { return m_hwnd; }


void Window::addListener(AbstractListener* listener)
{
    m_listener_list.push_back(listener);
}

void Window::processMessages() const
{
    MSG msg;

    for (auto& e : window_pool)
    {
        BOOL res = PeekMessage(&msg, e.first, 0, 0, PM_REMOVE);
        if (res)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

bool Window::update() const
{
    RECT rect{ 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    BOOL res = InvalidateRect(m_hwnd, &rect, FALSE);
    res |= UpdateWindow(m_hwnd);
    return static_cast<bool>(res);
}

void Window::adjustClientAreaSize()
{
    RECT window_rectangle{};
    window_rectangle.left = m_pos_x;
    window_rectangle.top = m_pos_y;
    window_rectangle.bottom = m_pos_x + m_width;
    window_rectangle.right = m_pos_y + m_height;

    if (!AdjustWindowRect(&window_rectangle, m_window_style.getValue(), FALSE))
    {
        std::string error_string = getLastSystemError();
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, error_string);
    }

    m_pos_x = window_rectangle.left;
    m_pos_y = window_rectangle.top;
    m_width = window_rectangle.right - window_rectangle.left;
    m_height = window_rectangle.bottom - window_rectangle.top;
}


LRESULT Window::WindowProcedure(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        Window* p_window = reinterpret_cast<Window*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
        if (p_window)
        {
            window_pool[hWnd] = p_window;
            return 0;
        }
        else
            return -1;
    }

    case WM_CLOSE:
    {
        window_pool[hWnd]->m_should_close = true;
        return 0;
    }

    case WM_DESTROY:
    {
        window_pool.erase(hWnd);
        return 0;
    }

    default:
    {
        Window* p_window = window_pool[hWnd];
        int64_t success_status = DefWindowProc(hWnd, uMsg, wParam, lParam);
        if (p_window == nullptr) return static_cast<LRESULT>(success_status);

        for (auto listener : p_window->m_listener_list)
        {
            int64_t status = listener->handle(uMsg, reinterpret_cast<uint64_t>(p_window), wParam, lParam, 0, 0, 0, 0, 0);

            if (status != AbstractListener::not_supported && status != success_status)
                return static_cast<LRESULT>(status);
        }

        return static_cast<LRESULT>(success_status);
    }
    }
}


