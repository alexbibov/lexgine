#ifndef LEXGINE_CORE_MISC_HASH_VALUE_H
#define LEXGINE_CORE_MISC_HASH_VALUE_H

#include <string>
#include <vector>
#include <compare>

namespace lexgine::core::misc 
{

class HashValue
{
public:
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
	virtual uint8_t hashWidth() const = 0;
	virtual uint8_t const* hashValue() const = 0;
	virtual std::strong_ordering operator<=>(HashValue const& other) const = 0;
	virtual bool operator==(HashValue const& other) const = 0;
};

}

#endif