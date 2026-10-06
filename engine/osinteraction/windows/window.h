// Implements wrapper over a window in Microsoft Windows and initializes DirectX 12 context over it

#ifndef LEXGINE_OSINTERACTION_WINDOWS_WINDOW_H
#define LEXGINE_OSINTERACTION_WINDOWS_WINDOW_H

#include <windows.h>
#include <dxgi1_5.h>
#include <d3d12.h>
#include <cstdint>
#include <string>
#include <deque>
#include <vector>


#include "engine/preprocessing/preprocessor_tokens.h"
#include "engine/core/math/vector_types.h"
#include "engine/core/math/rectangle.h"
#include "engine/core/misc/flags.h"
#include "engine/core/entity.h"

#include "engine/osinteraction/listener.h"

namespace lexgine::osinteraction::windows {

//! Kinds of the window events, which are recorded by the window procedure for a later deferred handling
enum class WindowEventType
{
    resized,    //!< client area of the window has been resized
    minimized,    //!< the window has been minimized and has no client area to render into
    maximized,    //!< the window has been maximized
    restored,    //!< the window has been restored from the minimized or maximized state
    dpi_changed,    //!< the window has been moved to a display with a different dots per inch setting
    close    //!< the user has requested the window to close
};


//! Window event recorded by the window procedure. The payload fields are only meaningful for certain event types
struct WindowEvent
{
    WindowEventType type;    //!< kind of the event
    uint32_t width;    //!< new width of the client area; only set for resized, maximized and restored events
    uint32_t height;    //!< new height of the client area; only set for resized, maximized and restored events
    uint32_t dpi;    //!< new dots per inch setting of the window; only set for dpi_changed events
};


class LEXGINE_CPP_API Window final : public core::NamedEntity<Window>
{
public:
    LEXGINE_CPP_API BEGIN_FLAGS_DECLARATION(WindowStyle)    //! Window style flags
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
        END_FLAGS_DECLARATION(WindowStyle);

    LEXGINE_CPP_API BEGIN_FLAGS_DECLARATION(WindowExStyle)    //! Window extended style flags
        FLAG(client_edge, WS_EX_CLIENTEDGE)    //!< the window has a border with a sunken edge
        FLAG(double_border, WS_EX_DLGMODALFRAME)    //!< the window has double border
        FLAG(_3d_border, WS_EX_STATICEDGE)    //!< the window has three-dimensional looking border style
        FLAG(raise_border_edge, WS_EX_WINDOWEDGE)    //!< the window has a border with raised edge
        FLAG(topmost, WS_EX_TOPMOST)    //!< the window should be put on top of all windows that were created without this flag set
        END_FLAGS_DECLARATION(WindowExStyle);

    Window(HINSTANCE hInstance = NULL,
        WindowStyle window_style = WindowStyle::base_values::thin_border | WindowStyle::base_values::has_minimize_box | WindowStyle::base_values::visible,
        WindowExStyle window_ex_style = {});


    ~Window();


    LEXGINE_CPP_API void setTitle(std::wstring const& title);    //! sets new title for the window
    LEXGINE_CPP_API std::wstring getTitle() const;    //! gets current title of the window

    LEXGINE_CPP_API void setDimensions(uint32_t width, uint32_t height);    //! sets new dimensions for the window
    LEXGINE_CPP_API void setDimensions(core::math::Vector2u const& dimensions);    //! sets new dimensions for the window. The new dimensions are packed into two-dimensional vector (width, height) in this order
    LEXGINE_CPP_API core::math::Vector2u getDimensions() const;    //! returns current dimensions of the window

    LEXGINE_CPP_API core::math::Rectangle getClientArea() const;    //! returns client area of the window

    LEXGINE_CPP_API void setLocation(uint32_t x, uint32_t y);    //! sets new location of the top-left corner of the window
    LEXGINE_CPP_API void setLocation(core::math::Vector2u const& location);    //! sets new location of the top-left corner of the window. The new location (x, y) in this order is packed into two-dimensional vector
    LEXGINE_CPP_API core::math::Vector2u getLocation() const;    //! returns current location of the top-left corner of the window represented in screen space coordinates

    LEXGINE_CPP_API void setVisibility(bool visibility_flag);    //! sets visibility of the window
    LEXGINE_CPP_API bool getVisibility() const;    //! returns visibility flag of the window

    LEXGINE_CPP_API bool shouldClose() const;    //! returns 'true' when user attempts to close the window

    HWND native() const { return m_hwnd; }    //! returns native window handler (HWND)

    void addListener(std::weak_ptr<AbstractListener> const& listener);    //! adds new event listener to the window. For design simplicity the listeners once added can never be removed without re-initialization of the Window object

    void processMessages() const;    //! retrieves messages addressed to the window from the message queue and dispatches them to the window

    /*! returns the events recorded by the window procedure since the last call of clearPendingEvents().

     The event queue is filled from the window procedure and is therefore only safe to touch from the thread,
     which pumps the messages of this window.
    */
    std::vector<WindowEvent> const& pendingEvents() const { return m_pending_events; }

    void clearPendingEvents();    //! drops the events recorded by the window procedure

private:
    void adjustClientAreaSize(core::math::Vector2u const& desired_position, core::math::Vector2u const& desired_dimensions,
        core::math::Vector2u& adjusted_position, core::math::Vector2u& adjusted_dimensions);    //! adjusts window size to accommodate the requested client area
    inline void checkSystemCall(bool result) const;    //! if the value of result is 'false', puts the window to erroneous state and logs the corresponding system error
    void pushEvent(WindowEvent const& event);    //! records an event for a later deferred handling, dropping the oldest one if the queue is full
    void recordEvent(UINT message, WPARAM w_param, LPARAM l_param);    //! translates a window message into a WindowEvent and records it, if the message describes one

private:
    // Window properties

    static constexpr size_t c_max_pending_events = 256;    //!< upper bound on the number of the events awaiting to be handled

    static ATOM m_atom;    //!< atom of the window class. Shared by all windows in the application (however, in majority of cases it is expected that the engine will just create a single window to which it is going to render)
    static uint32_t m_window_counter;    //!< counts total number of windows created by the application

    HINSTANCE m_hinstance;	//!< handle of the instance owning the window
    HWND m_hwnd;	//!< handle of the window
    WindowStyle m_window_style;	//!< window style bitset
    WindowExStyle m_window_ex_style;	//!< extended window style description
    bool m_should_close;	//!< equals 'true' if the window is ready to close. Equals 'false' otherwise
    std::deque<std::weak_ptr<AbstractListener>> m_listener_list;    //!< list of window listeners
    std::vector<WindowEvent> m_pending_events;    //!< events recorded by the window procedure and awaiting to be handled


    //! Window callback procedure shared by all windows in the application
    static LRESULT CALLBACK WindowProcedure(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
};

}

#endif