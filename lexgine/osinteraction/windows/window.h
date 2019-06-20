//Implements wrapper over a window in Microsoft Windows and initializes DirectX 12 context over it

#ifndef LEXGINE_OSINTERACTION_WINDOWS_WINDOW_H
#define LEXGINE_OSINTERACTION_WINDOWS_WINDOW_H

#include <windows.h>
#include <dxgi1_5.h>
#include <d3d12.h>
#include <cstdint>
#include <string>
#include <list>
#include <vector>


#include "lexgine/core/math/vector_types.h"
#include "lexgine/core/math/rectangle.h"
#include "lexgine/core/misc/flags.h"
#include "lexgine/core/entity.h"
#include "lexgine/core/class_names.h"

#include "lexgine/osinteraction/listener.h"

namespace lexgine::osinteraction::windows {


class Window final : public core::NamedEntity<lexgine::core::class_names::OSWindows_Window>
{
public:
    BEGIN_FLAGS_DECLARATION(WindowStyle)    //! Window style flags
    FLAG(thin_border, WS_BORDER)    //!< the window has a thin-line border
    FLAG(has_title_bar, WS_CAPTION)    //!< the window has a title bar
    FLAG(dialog_box_frame, WS_DLGFRAME)    //!< the window has a border of style typically used with dialog boxes. A window with this style cannot have a title bar
    FLAG(initially_minimized, WS_MINIMIZE)    //!< the window is initially minimized
    FLAG(initially_maximized, WS_MAXIMIZE)    //!< the window is initially maximized
    FLAG(has_minimize_box, WS_MINIMIZEBOX)    //!< the window has a minimize button (HasSystemMenu and HasTitleBar are automatically added)
    FLAG(has_maximize_box, WS_MAXIMIZEBOX)    //!< the window has a maximize button (HasSystemMenu and HasTitleBar are automatically added)
    FLAG(supports_sizing, WS_THICKFRAME)    //!< the window supports resizing
    FLAG(has_system_menu, WS_SYSMENU)    //!< the window has a system menu on its title bar (HasTitleBar is automatically added)
    FLAG(tiled, WS_OVERLAPPED)    //!< the window has a title bar and a border
    FLAG(visible, WS_VISIBLE)    //!< the window is initially visible
    END_FLAGS_DECLARATION(WindowStyle)

    BEGIN_FLAGS_DECLARATION(WindowExStyle)    //! Window extended style flags
    FLAG(client_edge, WS_EX_CLIENTEDGE)    //!< the window has a border with a sunken edge
    FLAG(double_border, WS_EX_DLGMODALFRAME)    //!< the window has double border
    FLAG(_3d_border, WS_EX_STATICEDGE)    //!< the window has three-dimensional looking border style
    FLAG(raise_border_edge, WS_EX_WINDOWEDGE)    //!< the window has a border with raised edge
    FLAG(topmost, WS_EX_TOPMOST)    //!< the window should be put on top of all windows that were created without this flag set
    END_FLAGS_DECLARATION(WindowExStyle)


    Window(HINSTANCE hInstance = NULL,
        WindowStyle window_style = WindowStyle::base_values::has_system_menu | WindowStyle::base_values::has_minimize_box
        | WindowStyle::base_values::has_maximize_box | WindowStyle::base_values::supports_sizing,
        WindowExStyle window_ex_style = WindowExStyle::base_values::raise_border_edge);

    ~Window();

    void setTitle(std::wstring const& title);    //! sets new title for the window
    std::wstring getTitle() const;    //! gets current title of the window

    void setDimensions(uint32_t width, uint32_t height);    //! sets new dimensions for the window
    void setDimensions(core::math::Vector2u const& dimensions);    //! sets new dimensions for the window. The new dimensions are packed into two-dimensional vector (width, height) in this order
    core::math::Vector2u getDimensions() const;    //! returns current dimensions of the window

    core::math::Rectangle getClientArea() const;    //! returns client area of the window

    void setLocation(uint32_t x, uint32_t y);    //! sets new location of the top-left corner of the window
    void setLocation(core::math::Vector2u const& location);    //! sets new location of the top-left corner of the window. The new location (x, y) in this order is packed into two-dimensional vector
    core::math::Vector2u getLocation() const;    //! returns current location of the top-left corner of the window represented in screen space coordinates

    void setVisibility(bool visibility_flag);    //! sets visibility of the window
    bool getVisibility() const;    //! returns visibility flag of the window

    bool shouldClose() const;    //! returns 'true' when user attempts to close the window

    HWND native() const;    //! returns native window handler (HWND)

    void addListener(std::weak_ptr<AbstractListener> const& listener);    //! adds new event listener to the window. For design simplicity the listeners once added can never be removed without re-initialization of the Window object

    void processMessages() const;    //! retrieves messages addressed to the window from the message queue and dispatches them to the window

    bool update() const;    //! updates window and returns 'true' on success

private:
    void adjustClientAreaSize();    //! adjust window size to accomodate the requested client area

private:
    // Window properties

    static ATOM m_atom;    //!< atom of the window class. Shared by all windows in the application (however, in majority of cases it is expected that the engine will just create a single window to which it is going to render)
    static uint32_t m_window_counter;    //!< counts total number of windows created by the application

    HINSTANCE m_hinstance;	//!< handle of the instance owning the window
    HWND m_hwnd;	//!< handle of the window
    WindowStyle m_window_style;	//!< window style bitset
    WindowExStyle m_window_ex_style;	//!< extended window style description
    std::wstring m_title;	//!< string title of the window
    uint32_t m_pos_x, m_pos_y;	//!< position of the upper-left corner of the window represented in screen coordinates
    uint32_t m_width, m_height;	//!< width and height of the window defined in pixels
    bool m_is_visible;	//!< equals 'true' if the window is visible. Equals 'false' otherwise
    bool m_should_close;	//!< equals 'true' if the window is ready to close. Equals 'false' otherwise
    std::list<std::weak_ptr<AbstractListener>> m_listener_list;    //!< list of window listeners


    //! Window callback procedure shared by all windows in the application
    static LRESULT CALLBACK WindowProcedure(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
};

}

#endif