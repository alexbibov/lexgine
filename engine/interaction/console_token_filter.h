#ifndef LEXGINE_INTERACTION_CONSOLE_CONSOLE_TOKEN_FILTER_H
#define LEXGINE_INTERACTION_CONSOLE_CONSOLE_TOKEN_FILTER_H

#include <vector>
#include <string>

namespace lexgine::interaction::console 
{

class ConsoleTokenFilter 
{
public:
    void addToken(std::string_view const& token, size_t id);

private:
    struct TokenEntry
    {
        std::string_view token;
        size_t id;
    };

private:
    std::vector<TokenEntry> m_tokens;
    std::vector<size_t> m_sorted_token_indices;
};

}

#endif