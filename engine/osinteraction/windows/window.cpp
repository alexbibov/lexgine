#include <algorithm>
#include <map>

#define OEMRESOURCE

#include "engine/core/exception.h"
#include "engine/core/entity.h"
#include "window.h"

using namespace lexgine;
using namespace lexgine::osinteraction;
using namespace lexgine::osinteraction::windows;


ATOM Window::m_atom = 0;
uint32_t Window::m_window_counter = 0;


namespace {

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
    Window::WindowStyle::int_type window_style_flags{};
    Window::WindowExStyle::int_type window_ex_style_flags = static_cast<Window::WindowExStyle::int_type>(window_extended_style);

    if (window_style.isSet(Window::WindowStyle::base_values::has_maximize_box) ||
        window_style.isSet(Window::WindowStyle::base_values::has_minimize_box))
        window_style_flags = static_cast<Window::WindowStyle::int_type>(window_style | Window::WindowStyle::base_values::has_system_menu);

    if (window_style.isSet(Window::WindowStyle::base_values::has_system_menu))
        window_style_flags |= static_cast<Window::WindowStyle::int_type>(Window::WindowStyle::base_values::has_title_bar);


    // Create window class if it has not been provided by the caller
    if (!atom)
    {
        WNDCLASSEX wndclassex;
        wndclassex.cbSize = sizeof(WNDCLASSEX);
        wndclassex.style = CS_DBLCLKS | CS_OWNDC;
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

    return CreateWindowEx(window_ex_style_flags, const_cast<wchar_t*>(window_class_name), title.c_str(), window_style_flags, x, y, width, height,
        NULL, NULL, hInstance, static_cast<LPVOID>(p_window));
}


}


Window::Window(HINSTANCE hInstance /* = NULL */,
    WindowStyle window_style /* = WindowStyle
                             | WindowStyle::base_values::HasSystemMenu
                             | WindowStyle::base_values::HasMinimizeButton
                             | WindowStyle::base_values::HasMaximizeButton
                             | WindowStyle::base_values::SupportsSizing */,
    WindowExStyle window_ex_style /* = WindowExStyle::base_values::RaisedBorderEdge */) :
    m_hinstance{ hInstance ? hInstance : GetModuleHandle(NULL) },
    m_hwnd{ NULL },
    m_window_style{ window_style },
    m_window_ex_style{ window_ex_style },
    m_should_close{ false }
{
    core::math::Vector2u const default_position{ 0U, 0U };
    core::math::Vector2u const default_dimensions{ 1280U, 1024U };

    core::math::Vector2u adjusted_position{}, adjusted_dimensions{};

    adjustClientAreaSize(default_position, default_dimensions, adjusted_position, adjusted_dimensions);

    m_hwnd = createWindow(this, m_atom, m_hinstance,
        adjusted_position.x, adjusted_position.y,
        adjusted_dimensions.x, adjusted_dimensions.y,
        m_window_style, m_window_ex_style, L"LexgineDXWindow", WindowProcedure);

    checkSystemCall(m_hwnd != 0);

    ++m_window_counter;
}

Window::~Window()
{
    checkSystemCall(DestroyWindow(m_hwnd));

    if (!m_window_counter && m_atom)
    {
        checkSystemCall(UnregisterClass(const_cast<wchar_t*>(window_class_name), m_hinstance));
    }
}

void Window::setTitle(std::wstring const& title)
{
    checkSystemCall(SetWindowText(m_hwnd, title.c_str()));
}


std::wstring Window::getTitle() const
{
    int title_length = GetWindowTextLength(m_hwnd);
    checkSystemCall(title_length != 0);

    std::vector<wchar_t> text(title_length);
    checkSystemCall(GetWindowText(m_hwnd, text.data(), title_length));

    return std::wstring{ text.data() };
}


void Window::setDimensions(uint32_t width, uint32_t height)
{
    if (!m_hwnd) return;
    RECT wndrect{};
    checkSystemCall(GetWindowRect(m_hwnd, &wndrect));

    core::math::Vector2u adjusted_position{}, adjusted_dimension{};
    adjustClientAreaSize(core::math::Vector2u{ static_cast<uint32_t>(wndrect.left), static_cast<uint32_t>(wndrect.top) },
        core::math::Vector2u{ width, height }, adjusted_position, adjusted_dimension);

    checkSystemCall(SetWindowPos(m_hwnd, NULL, static_cast<int>(adjusted_position.x), static_cast<int>(adjusted_position.y),
        static_cast<int>(adjusted_dimension.x), static_cast<int>(adjusted_dimension.y), SWP_ASYNCWINDOWPOS | SWP_NOZORDER));
}


void Window::setDimensions(core::math::Vector2u const& dimensions)
{
    setDimensions(dimensions.x, dimensions.y);
}

core::math::Vector2u Window::getDimensions() const
{
    RECT wndrect{};
    checkSystemCall(GetWindowRect(m_hwnd, &wndrect));
    return core::math::Vector2u{
        static_cast<uint32_t>(wndrect.right - wndrect.left),
        static_cast<uint32_t>(wndrect.bottom - wndrect.top)
    };
}

core::math::Rectangle Window::getClientArea() const
{
    RECT rect{};
    checkSystemCall(GetClientRect(m_hwnd, &rect));

    return core::math::Rectangle{
        core::math::Vector2f{static_cast<float>(rect.left), static_cast<float>(rect.top)},
        static_cast<float>(rect.right - rect.left), static_cast<float>(rect.bottom - rect.top)
    };
}

void Window::setLocation(uint32_t x, uint32_t y)
{
    RECT rect{};
    checkSystemCall(GetWindowRect(m_hwnd, &rect));
    checkSystemCall(SetWindowPos(m_hwnd, NULL, static_cast<int>(x), static_cast<int>(y),
        static_cast<int>(rect.right - rect.left), static_cast<int>(rect.bottom - rect.top),
        SWP_ASYNCWINDOWPOS | SWP_NOZORDER));
}


void Window::setLocation(core::math::Vector2u const& location)
{
    setLocation(location.x, location.y);
}


core::math::Vector2u Window::getLocation() const
{
    RECT rect{};
    checkSystemCall(GetWindowRect(m_hwnd, &rect));
    return core::math::Vector2u{ static_cast<uint32_t>(rect.left), static_cast<uint32_t>(rect.top) };
}


void Window::setVisibility(bool visibility_flag)
{
    ShowWindow(m_hwnd, visibility_flag ? SW_SHOW : SW_HIDE);
}


bool Window::getVisibility() const
{
    return IsWindowVisible(m_hwnd);
}


bool Window::shouldClose() const { return  m_should_close; }


void Window::addListener(std::weak_ptr<AbstractListener> const& listener)
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

void Window::update() const
{
    RECT rect{};
    checkSystemCall(GetClientRect(m_hwnd, &rect));
    checkSystemCall(InvalidateRect(m_hwnd, &rect, FALSE));
    checkSystemCall(UpdateWindow(m_hwnd));
}

void Window::adjustClientAreaSize(core::math::Vector2u const& desired_position, core::math::Vector2u const& desired_dimensions,
    core::math::Vector2u& adjusted_position, core::math::Vector2u& adjusted_dimensions)
{
    RECT window_rectangle{};
    window_rectangle.left = static_cast<LONG>(desired_position.x);
    window_rectangle.top = static_cast<LONG>(desired_position.y);
    window_rectangle.right = static_cast<LONG>(desired_position.x + desired_dimensions.x);
    window_rectangle.bottom = static_cast<LONG>(desired_position.y + desired_dimensions.y);

    checkSystemCall(AdjustWindowRect(&window_rectangle, m_window_style.getValue(), FALSE));

    adjusted_position = core::math::Vector2u{
        static_cast<unsigned>(window_rectangle.left),
        static_cast<unsigned>(window_rectangle.top)
    };

    adjusted_dimensions = core::math::Vector2u{
        static_cast<unsigned>(window_rectangle.right - window_rectangle.left),
        static_cast<unsigned>(window_rectangle.bottom - window_rectangle.top)
    };
}

inline void Window::checkSystemCall(bool result) const
{
    if (!result)
    {
        std::string error_string = getLastSystemError();
        LEXGINE_THROW_ERROR_FROM_NAMED_ENTITY(this, error_string);
    }
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
        if (p_window == nullptr)
        {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }

        for (auto listener : p_window->m_listener_list)
        {
            if (auto ptr = listener.lock())
            {
                MessageHandlingResult result = ptr->handle(uMsg, reinterpret_cast<uint64_t>(p_window), wParam, lParam, 0, 0, 0, 0, 0);
                if (result == MessageHandlingResult::fail)
                {
                    p_window->logger().out(
                        std::format("Failed to handle message {} for window {}", uMsg, p_window->getStringName()),
                        core::misc::LogMessageType::exclamation
                    );
                }
            }
        }

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    }
}


