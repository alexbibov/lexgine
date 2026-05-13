#ifndef LEXGINE_CORE_MISC_HASHES_FNV1A_64_H
#define LEXGINE_CORE_MISC_HASHES_FNV1A_64_H

#include <cstdint>

#include "engine/core/misc/hash_value.h"

namespace lexgine::core::misc::hashes
{

class Fnv1a_64 : public HashValue
{
public:
    void create(void const* p_data, size_t data_size) override;
    void combine(void const* p_data, size_t data_size) override;
    void finalize() override {}
    uint8_t hashWidth() const override { return sizeof(m_hashData); }
    uint8_t const* hashValue() const override { return m_hashData.bytes; }
    std::strong_ordering operator<=>(HashValue const& other) const override
    {
        return this->operator<=>(*static_cast<Fnv1a_64 const*>(&other));
    }
    bool operator==(HashValue const& other) const override
    {
        return this->operator==(*static_cast<Fnv1a_64 const*>(&other));
    }
    std::strong_ordering operator<=>(Fnv1a_64 const& other) const;
    bool operator==(Fnv1a_64 const& other) const;
    uint64_t hashValueT() const { return m_hashData.value; }

private:
    static uint64_t constexpr m_offset_basis = 0xcbf29ce484222325ULL;
    static uint64_t constexpr m_prime        = 0x100000001b3ULL;

private:
    union
    {
        uint8_t bytes[sizeof(uint64_t)];
        uint64_t value;
    } m_hashData { .value = m_offset_basis };
};

}

#endif
