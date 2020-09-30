#ifndef LEXGINE_CORE_UI_UI_PROVIDER_H
#define LEXGINE_CORE_UI_UI_PROVIDER_H

#include <memory>
#include <utility>

namespace lexgine::core::ui {

class UIProvider : public std::enable_shared_from_this<UIProvider>
{
public:
    bool isEnabled() const { return m_is_enabled; }
    void setEnabledState(bool enabled) { m_is_enabled = enabled; }

    std::shared_ptr<UIProvider const> getPointer() const { return shared_from_this(); }

    virtual void constructUI() = 0;

private:
    bool m_is_enabled = true;
};

}

#endif
