#ifndef LEXGINE_CORE_UI_BOTTOM_MENU_H
#define LEXGINE_CORE_UI_BOTTOM_MENU_H

#include <memory>

#include "engine/core/lexgine_core_fwd.h"
#include "engine/core/ui/ui_provider.h"
#include "engine/core/ui/lexgine_core_ui_fwd.h"
#include "engine/osinteraction/windows/window_listeners.h"

namespace lexgine::core::ui
{

class BottomMenu final 
    : public UIProvider
    , public osinteraction::Listeners<osinteraction::windows::KeyInputListener>
{
public:
    static std::shared_ptr<BottomMenu> create(
        Globals const& globals
    );

public:    // required by UIProvider
    void constructUI() override;

public: // KeyInputListener	
    bool character(uint64_t char_key_code) override;  //! called when a character key is pressed; The should returns 'true' on success

private:
    std::unique_ptr<Profiler> m_profiler;
    std::unique_ptr<Console> m_console;
};

}

#endif