#ifndef LEXGINE_CORE_UI_H
#define LEXGINE_CORE_UI_H

#include <memory>
#include <utility>

namespace lexgine::core {

class UIProvider : public std::enable_shared_from_this<UIProvider>
{
public:
    bool isEnabled() const { return m_is_enabled; }
    void setEnablingStatus(bool enabling) { m_is_enabled = enabling; }

    std::shared_ptr<UIProvider const> getPointer() const { return shared_from_this(); }

    virtual void constructUI() = 0;

private:
    bool m_is_enabled = true;
};

}

#endif
