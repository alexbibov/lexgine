#ifndef LEXGINE_CORE_MISC_HASHES_XXHASH64_H
#define LEXGINE_CORE_MISC_HASHES_XXHASH64_H

#include <cstdint>
#include "xxhash.h"
#include "engine/core/misc/hash_value.h"

namespace lexgine::core::misc::hashes {

class XXHash64 final: public HashValue
{
public:
    XXHash64() = default;
    XXHash64(void const* p_data, size_t data_size);
    void create(void const* p_data, size_t data_size) override;
    void combine(void const* p_data, size_t data_size) override;
    void finalize() override;
    size_t hashWidth() const override { return sizeof(m_hash_data); }
    uint8_t const* hashValue() const override { return m_hash_data.bytes; }
    std::strong_ordering operator<=>(HashValue const& other) const override
    {
        return this->operator<=>(*static_cast<XXHash64 const*>(&other));
    }
    bool operator==(HashValue const& other) const override
    {
        return this->operator==(*static_cast<XXHash64 const*>(&other));
    }
    std::strong_ordering operator<=>(XXHash64 const& other) const;
    bool operator==(XXHash64 const& other) const;
    uint64_t fold() const override;
    uint64_t hashValueT() const { return m_hash_data.value; }

private:
    XXH3_state_t* m_xxh3_state{ nullptr };
    union
    {
        uint8_t bytes[sizeof(uint64_t)];
        uint64_t value;
    } m_hash_data{};
};

}

#endif
