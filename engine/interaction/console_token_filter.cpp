#include "console_token_filter.h"

namespace lexgine::interaction::console
{

void ConsoleTokenFilter::addToken(std::string const& token, size_t id)
{
    if (m_tokens.empty())
    {
        m_tokens.push_back({ token, id });
    } 
    else
    {
        std::vector<char> lower_token {};
        std::transform(
            token.begin(), 
            token.end(), 
            std::back_inserter(lower_token),
            [](char e)
            {
                return static_cast<char>(std::tolower(e));
            }
        );
        auto it = std::lower_bound(
            m_tokens.begin(),
            m_tokens.end(), 
            std::string { lower_token.begin(), lower_token.end() }, 
            [](TokenEntry const& entry, std::string const& value) {
                return entry.token < value;
            }
        );
        m_tokens.insert(it, { token, id });
    }
}

}