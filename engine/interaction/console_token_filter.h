#ifndef LEXGINE_INTERACTION_CONSOLE_CONSOLE_TOKEN_FILTER_H
#define LEXGINE_INTERACTION_CONSOLE_CONSOLE_TOKEN_FILTER_H

#include <vector>
#include <string>
#include <iterator>
#include <algorithm>
#include <ranges>

namespace lexgine::interaction::console 
{

class ConsoleTokenFilter 
{
public:
    void addToken(std::string const& token, size_t id);
    [[nodiscard]] auto filter(std::string const& filter_prefix) const
    {
        auto lb = std::lower_bound(
            m_tokens.begin(), 
            m_tokens.end(), 
            filter_prefix, 
            [](TokenEntry const& entry, std::string_view const& value) {
                return entry.token < value;
            }
        );

        auto ub = std::lower_bound(
            m_tokens.begin(),
            m_tokens.end(),
            filter_prefix + '\x7f',  // append a character that is lexicographically greater than any valid character to find the upper bound
            [](TokenEntry const& entry, std::string_view const& value) {
                return entry.token < value;
            }
        );

        std::ptrdiff_t drop_count = std::distance(m_tokens.begin(), lb);
        std::ptrdiff_t take_count = lb != m_tokens.end() ? std::distance(lb, ub) : 0;
        if (lb == ub)
        {
            drop_count = static_cast<std::ptrdiff_t>(m_tokens.size());
            take_count = 0;
        }
        return std::views::drop(m_tokens, drop_count) 
            | std::views::take(take_count) 
            | std::views::transform([](TokenEntry const& entry) { return entry.id; });
    }

private:
    struct TokenEntry
    {
        std::string token;
        size_t id;
    };

private:
    std::vector<TokenEntry> m_tokens;
    std::vector<size_t> m_sorted_token_indices;
};

}

#endif