#ifndef LEXGINE_CORE_MISC_UUID_H
#define LEXGINE_CORE_MISC_UUID_H

#include <cstdint>
#include <compare>
#include <string>

namespace lexgine::core::misc {

class UUID final
{
public:
    UUID()
        : m_lo_part{ 0 }, m_hi_part{ 0 }
    {
    }

    UUID(uint64_t lo_part, uint64_t hi_part)
        : m_lo_part{ lo_part }, m_hi_part{ hi_part }
    {
    }

    UUID(std::string const& uuid_string);

    auto operator<=>(UUID const& other) const = default;

    std::string toString() const;

    static UUID generate();

    uint64_t loPart() const { return m_lo_part; }
    uint64_t hiPart() const { return m_hi_part; }

private:
    uint64_t m_lo_part;
    uint64_t m_hi_part;
};

}

#endif