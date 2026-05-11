#include "blake3_256.h"

namespace lexgine::core::misc::hashes
{
void Blake3_256::create(void const* p_data, size_t data_size)
{
	blake3_hasher_init(&m_blake3_hasher);
	blake3_hasher_update(&m_blake3_hasher, p_data, data_size);
}

void Blake3_256::combine(void const* p_data, size_t data_size)
{
	blake3_hasher_update(&m_blake3_hasher, p_data, data_size);
}


void Blake3_256::finalize()
{
	blake3_hasher_finalize(&m_blake3_hasher, m_hashData.data(), sizeof(m_hashData));
}

std::strong_ordering Blake3_256::operator<=>(Blake3_256 const& other) const
{
	return m_hashData <=> other.m_hashData;
}

bool Blake3_256::operator==(Blake3_256 const& other) const
{
	return m_hashData == other.m_hashData;
}
}