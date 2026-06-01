#ifndef LEXGINE_CORE_MISC_HASHES_BLAKE3_256_H
#define LEXGINE_CORE_MISC_HASHES_BLAKE3_256_H

#include <array>

#include "blake3.h"

#include "engine/core/misc/hash_value.h"

namespace lexgine::core::misc::hashes
{
class Blake3_256 final : public HashValue
{
public:
    void create(void const* p_data, size_t data_size) override;
    void combine(void const* p_data, size_t data_size) override;
    void finalize() override;
    size_t hashWidth() const override { return sizeof(m_hash_data); }
    uint8_t const* hashValue() const override { return m_hash_data.data(); };
    std::strong_ordering operator<=>(HashValue const& other) const override
    {
        return this->operator<=>(*static_cast<Blake3_256 const*>(&other));
    }
    bool operator==(HashValue const& other) const override
    {
        return this->operator==(*static_cast<Blake3_256 const*>(&other));
    }
    std::strong_ordering operator<=>(Blake3_256 const& other) const;
    bool operator==(Blake3_256 const& other) const;
    uint64_t fold() const override;
    std::array<uint8_t, 32> const& hashValueT() const { return m_hash_data; }

private:
    blake3_hasher m_blake3_hasher;
    std::array<uint8_t, 32> m_hash_data{};
};
}

#endif
