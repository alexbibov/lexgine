#include "console_token_filter.h"

namespace lexgine::interaction::console
{

void ConsoleTokenFilter::addToken(std::string_view const& token, size_t id)
{
    if (m_tokens.empty())
    {
        m_tokens.push_back({ token, id });
    } 
    else
    {
        auto it = std::lower_bound(m_tokens.begin(), m_tokens.end(), token, 
            [](const TokenEntry& entry, const std::string_view& value) {
                return entry.token < value;
            });
        m_tokens.insert(it, { token, id });
    }
}

}