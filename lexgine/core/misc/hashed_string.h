#ifndef LEXGINE_CORE_MISC_HASHED_STRING_H
#define LEXGINE_CORE_MISC_HASHED_STRING_H

#include <string>

#include "../../../3rd_party/SpookyHash/SpookyV2.h"

namespace lexgine {namespace core {namespace misc {

class HashedString
{
private:
    std::string m_string;
    uint64_t m_hash;

public:
    HashedString(spooky_hash_v2::SpookyHash& hash_generator, uint64_t seed, std::string const& str);

    bool operator<(HashedString const& other) const;
    bool operator==(HashedString const& other) const;

    uint64_t hash() const;
    char const* string() const;
};

}}}

#endif
