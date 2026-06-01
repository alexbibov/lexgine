#ifndef LEXGINE_CORE_MISC_HASH_VALUE_H
#define LEXGINE_CORE_MISC_HASH_VALUE_H

#include <string>
#include <vector>
#include <compare>
#include <cstdint>
#include <cstring>

namespace lexgine::core::misc 
{

class HashValue
{
public:
    virtual ~HashValue() = default;
    virtual void create(void const* data, size_t data_size) = 0;
    void create(std::vector<uint8_t> const& data)
    {
        create(data.data(), data.size());
    }
    void combine(std::vector<uint8_t> const& data)
    {
        combine(data.data(), data.size());
    }
    virtual void combine(void const* p_data, size_t data_size) = 0;
    virtual void finalize() = 0;
    virtual size_t hashWidth() const = 0;
    virtual uint8_t const* hashValue() const = 0;
    virtual std::strong_ordering operator<=>(HashValue const& other) const = 0;
    virtual bool operator==(HashValue const& other) const = 0;
    virtual uint64_t fold() const = 0;
    bool isInitialized() const { return m_initialized; }
    bool isFinalized() const { return m_finalized; }

protected:
    static uint64_t rotateLeft(uint64_t value, uint32_t shift)
    {
        shift &= 63;
        return shift ? (value << shift) | (value >> (64 - shift)) : value;
    }

    static uint64_t foldBytes(uint8_t const* data, size_t size)
    {
        uint64_t rv{};
        size_t offset{};
        uint32_t rotation{ 0 };
        for (; offset + sizeof(uint64_t) <= size; offset += sizeof(uint64_t))
        {
            uint64_t word{};
            std::memcpy(&word, data + offset, sizeof(word));
            rv ^= rotateLeft(word, rotation);
            rotation = (rotation + 17) & 63;
        }

        if (offset < size)
        {
            uint64_t word{};
            std::memcpy(&word, data + offset, size - offset);
            rv ^= rotateLeft(word, rotation);
        }

        return rv;
    }

protected:
    bool m_initialized{ false };
    bool m_finalized{ false };
};

}

#endif
