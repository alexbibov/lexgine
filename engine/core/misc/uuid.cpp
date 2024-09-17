#include <mutex>
#include <random>
#include <cassert>

#include "uuid.h"

namespace lexgine::core::misc {

namespace
{

std::random_device rd{};
std::uniform_int_distribution<uint64_t> distribution{};
std::mutex rg_mutex{};

uint8_t hex_octet_to_int(char c0, char c1)
{
    uint8_t digit1 = c0 >= '0' && c0 <= '9' ? c0 - '0' : c0 - 'a' + 10;
    uint8_t digit2 = c1 >= '0' && c1 <= '9' ? c1 - '0' : c1 - 'a' + 10;
    return digit1 * 16 + digit2;
}

void int_to_hex_octet(uint8_t value, char& c0, char& c1)
{
    uint8_t digit1 = value / 16;
    uint8_t digit2 = value % 16;
    c0 = digit1 < 10 ? '0' + digit1 : 'a' + digit1 - 10;
    c1 = digit2 < 10 ? '0' + digit2 : 'a' + digit2 - 10;
}


}

UUID::UUID(std::string const& uuid_string)
    : m_lo_part{ 0 }, m_hi_part{ 0 }
{
    assert(uuid_string.size() == 36);
    for (size_t i = 0; i < 36; i+= 2)
    {
        if (i == 8 || i == 13 || i == 18 || i == 23)
        {
            assert(uuid_string[i] == '-');
            --i;
        }
        else
        {
            uint8_t digit = hex_octet_to_int(uuid_string[i], uuid_string[i + 1]);
            if (i < 18)
            {
                m_hi_part = (m_hi_part << 8) | digit;
            }
            else
            {
                m_lo_part = (m_lo_part << 8) | digit;
            }
        }
    }
}

std::string UUID::toString() const
{
    char c[36];

    size_t offset{ 0 };
    for (size_t i = 0; i < 8; ++i)
    {
        int_to_hex_octet((m_hi_part >> (56 - i * 8)) & 0xFF, c[offset], c[offset + 1]);
        offset += 2;

        if (i == 3 || i == 5)
        {
            c[offset] = '-';
            ++offset;
        }
    }

    c[offset++] = '-';

    for (size_t i = 0; i < 8; ++i)
    {
        int_to_hex_octet((m_lo_part >> (56 - i * 8)) & 0xFF, c[offset], c[offset + 1]);
        offset += 2;

        if (i == 1)
        {
            c[offset] = '-';
            ++offset;
        }
    }

    return std::string{ c, 36 };
}

UUID UUID::generate()
{
    std::scoped_lock<std::mutex> lock{ rg_mutex };
    return UUID{ distribution(rd), distribution(rd) };
}


}