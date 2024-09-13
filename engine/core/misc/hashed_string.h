#ifndef LEXGINE_CORE_MISC_HASHED_STRING_H
#define LEXGINE_CORE_MISC_HASHED_STRING_H

#include <string>

namespace lexgine::core::misc {

class HashedString
{
public:
    HashedString();
    HashedString(std::string const& str);

    bool operator<(HashedString const& other) const;
    bool operator==(HashedString const& other) const;

    uint64_t hash() const;
    char const* string() const;

private:
    std::string m_string;
    uint64_t m_hash;

};

}


// needed for simpler functioning of std::unordered_set and std::unordered_map
namespace std {
template<>
struct hash<lexgine::core::misc::HashedString>
{
    using argument_type = lexgine::core::misc::HashedString;
    using result_type = size_t;

    result_type operator()(argument_type const& key) const
    {
        return key.hash();
    }
};

}

#endif
