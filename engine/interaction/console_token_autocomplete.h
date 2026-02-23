#ifndef CONSOLE_TOKEN_AUTOCOMPLETE_H
#define CONSOLE_TOKEN_AUTOCOMPLETE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <numeric>

#include "console_token_filter.h"

namespace lexgine::interaction::console
{

class ConsoleTokenAutocomplete 
{
private:
	static constexpr size_t c_preReservedCommandCount = 100;
public:
	ConsoleTokenAutocomplete(std::size_t suggestion_count = 10)
		: m_suggestion_count{ suggestion_count } 
	{
		m_pool.reserve(c_preReservedCommandCount);  // pre-reserve space for 100 tokens
		m_buckets.reserve(c_preReservedCommandCount);    
	}

	void setSuggestionCount(std::size_t new_suggestion_count) { m_suggestion_count = new_suggestion_count; }
	void clearTokenPool();
	void addToken(const std::string& token_name);
	void clearQuery();
	void setQuery(const std::string_view& query);
	void append(char c, uint16_t tau = (std::numeric_limits<uint16_t>::max)());
	void backspace();
	std::vector<std::pair<std::string, uint16_t>> suggestions(size_t suggestion_count_override = 0) const;
	std::string_view query() const { return m_query; }
	std::vector<std::string> allTokens() const;

	static uint16_t computeDLDistance(std::string_view const& a, std::string_view const& b);

private:
	struct Candidate final{
		std::string name;
		std::vector<uint16_t> prev;   // DP row for |query| = i-1
		std::vector<uint16_t> prev2;  // DP row for |query| = i-2 (needed for OSA transposition)
		std::vector<uint16_t> curr;   // scratch row reused to avoid realloc
		uint16_t dist;                // current OSA distance
		// bucket bookkeeping
		size_t place_in_bucket;
		bool placed{ false };

		explicit Candidate(const std::string_view& command_name);
		uint16_t update(char lastQueryCharacter, char prevQueryCharacter, uint16_t tau);  // updates internal state of the candidate based on the last two characters of the query
		void reset();
		void rotateRows();
	};

private:
	void placeInBucket(size_t id, size_t d);
	void moveBucket(size_t id, size_t old_d, size_t new_d);

private:
	std::vector<Candidate> m_pool;
	std::vector<std::vector<size_t>> m_buckets; // buckets[d] = list of ids at distance d
	std::string m_query;
	size_t m_suggestion_count = 10;	
	ConsoleTokenFilter m_token_filter;
};

}

#endif
