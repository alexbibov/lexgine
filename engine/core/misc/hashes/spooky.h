#ifndef LEXGINE_CORE_MISC_HASHES_SPOOKY_HASH_VALUE_H
#define LEXGINE_CORE_MISC_HASHES_SPOOKY_HASH_VALUE_H

#include <string>
#include <vector>


#include "engine/core/misc/hash_value.h"
#include "3rd_party/SpookyHash/SpookyV2.h"

namespace lexgine::core::misc::hashes {

class SpookyHashValue : public HashValue
{
public:
    void create(void const* data, size_t data_size) override;
    void combine(void const* p_data, size_t data_size) override;
    void finalize() override {};
    uint8_t hashWidth() const override { return sizeof(m_hashData); }
    uint8_t const* hashValue() const override { return m_hashData.bytes; }
    std::strong_ordering operator<=>(HashValue const& other) const override
    {
        return this->operator<=>(*static_cast<SpookyHashValue const*>(&other));
    }
    bool operator==(HashValue const& other) const override
    {
        return this->operator==(*static_cast<SpookyHashValue const*>(&other));
    }
	std::strong_ordering operator<=>(SpookyHashValue const& other) const;
	bool operator==(SpookyHashValue const& other) const;
private:
    // the numbers for the seed values are taken from www.random.org
    static uint64_t const m_seed1 = 0x744dd29e659fecd9;
    static uint64_t const m_seed2 = 0x7f3a28db614e26d6;
    static thread_local spooky_hash_v2::SpookyHash m_hash_generator;

private:
    union 
    {
        uint8_t bytes[16];
        uint64_t words[2];
    }m_hashData;
};

}

#endif
