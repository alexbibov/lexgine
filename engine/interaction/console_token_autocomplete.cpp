#include <numeric>
#include <algorithm>
#include <array>
#include <iterator>
#include "console_token_autocomplete.h"

namespace lexgine::interaction::console
{ 

void ConsoleTokenAutocomplete::clearTokenPool()
{
	m_pool.clear();
	m_buckets.clear();
	m_query.clear();
}

void ConsoleTokenAutocomplete::addToken(const std::string& token_name)
{
	m_pool.push_back(Candidate{ token_name });
    m_token_filter.addToken(token_name);
	size_t id = m_pool.size() - 1;
	placeInBucket(id, m_pool[id].dist);
}

void ConsoleTokenAutocomplete::clearQuery()
{
	m_query.clear();
	m_buckets.clear();
	for (auto& c : m_pool) 
	{
		c.reset();
	}
	for (size_t id = 0; id < m_pool.size(); ++id) 
	{
		placeInBucket(id, m_pool[id].dist);
	}
}

void ConsoleTokenAutocomplete::setQuery(const std::string_view& query)
{
	clearQuery();
	m_query.reserve(query.length());
	for (char ch : query) 
	{
		append(ch);
	}
}

void ConsoleTokenAutocomplete::append(char c, uint16_t tau /*= std::numeric_limits<uint16_t>::max()*/)
{
	const char prev = m_query.empty() ? '\0' : m_query.back();
	m_query.push_back(c);

	// Update each candidate incrementally and move its bucket in O(1)
	for (size_t id = 0; id < m_pool.size(); ++id) {
		auto& cand = m_pool[id];
		const uint16_t old_d = cand.dist;
		const uint16_t new_d = cand.update(c, prev, tau);
		moveBucket(id, old_d, new_d);
	}
}

void ConsoleTokenAutocomplete::backspace()
{
	if (m_query.empty()) return;
	m_query.pop_back();

	// Rebuild everything from scratch for the shorter query
	m_buckets.clear();
	for (auto& c : m_pool)
	{
		c.reset();
	}

	// replay incrementally
	char prev = '\0';
	for (char ch : m_query) {
		for (size_t id = 0; id < m_pool.size(); ++id) {
			auto& cand = m_pool[id];
			const uint16_t old_d = cand.dist;
			const uint16_t new_d = cand.update(ch, prev, std::numeric_limits<uint16_t>::max());
			moveBucket(id, old_d, new_d);
		}
		prev = ch;
	}
}

std::vector<std::pair<std::string, uint16_t>> ConsoleTokenAutocomplete::suggestions(size_t suggestion_count_override /*= 0*/) const
{
	const size_t want = (suggestion_count_override ? suggestion_count_override : m_suggestion_count);
	std::vector<std::string> out;
	out.reserve(want);

	for (size_t d = 0; d < m_buckets.size() && out.size() < want; ++d) {
		for (auto id : m_buckets[d]) {
			out.emplace_back(m_pool[id].name);
			if (out.size() == want) break;
		}
	}
	// If we didn't fill K, scan any "overflow" distances beyond m_buckets.size()
	// (should be rare; distances are <= max(len(name), len(query)) )
	if (out.size() < want) 
	{
		// Fallback: compute remaining by linear scan, ordered by distance then stable id.
		// (No extra sorting; just do an extra pass.)
		for (size_t id = 0; id < m_pool.size() && out.size() < want; ++id) {
			auto d = m_pool[id].dist;
			if (d >= m_buckets.size()) {
				out.emplace_back(m_pool[id].name);
			}
		}
	}

	// Sort suggestions based on unrestricted DL-distance
	std::vector<std::pair<std::string, uint16_t>> weighted_outs(out.size());
	std::transform(out.begin(), out.end(), weighted_outs.begin(),
		[this](const std::string& s)
		{
			return std::make_pair(s, computeDLDistance(m_query, s));
		}
	);
	std::sort(weighted_outs.begin(), weighted_outs.end(),
		[this](const auto& lhs, const auto& rhs)
		{
			return lhs.second < rhs.second;
		}
	);
	return weighted_outs;
}

void ConsoleTokenAutocomplete::placeInBucket(size_t id, size_t d)
{
	if (d >= m_buckets.size()) m_buckets.resize(d + 1);
	m_buckets[d].reserve(std::max(m_pool.size(), c_preReservedCommandCount));
	m_buckets[d].push_back(id);
	m_pool[id].place_in_bucket = m_buckets[d].size() - 1;
	m_pool[id].placed = true;
}

void ConsoleTokenAutocomplete::moveBucket(size_t id, size_t old_d, size_t new_d)
{
	Candidate& cand = m_pool[id];
	if (!cand.placed) { placeInBucket(id, new_d); return; }
	if (old_d == new_d) return;

	// remove from old bucket in O(1)
	if (old_d < m_buckets.size()) 
	{
		std::swap(m_buckets[old_d][cand.place_in_bucket], m_buckets[old_d].back());
		m_pool[m_buckets[old_d][cand.place_in_bucket]].place_in_bucket = cand.place_in_bucket;
		cand.placed = false;
		m_buckets[old_d].pop_back();
	}

	// add to new bucket
	placeInBucket(id, new_d);
}

uint16_t ConsoleTokenAutocomplete::computeDLDistance(std::string_view const& a, std::string_view const& b)
{
	const size_t n = a.size();
	const size_t m = b.size();
	if (n == 0) return static_cast<uint16_t>(m);
	if (m == 0) return static_cast<uint16_t>(n);

	const uint16_t maxdist = static_cast<uint16_t>(n + m + 1);
	std::vector<uint16_t> d((n + 2) * (m + 2));

	d[0] = maxdist;
	for (size_t i = 0; i <= n; ++i)
	{
		d[(i + 1) * (m + 2)] = maxdist;
		d[(i + 1) * (m + 2) + 1] = static_cast<uint16_t>(i);
	}
	for (size_t j = 0; j <= m; ++j)
	{
		d[0 * (m + 2) + (j + 1)] = maxdist;
		d[1 * (m + 2) + (j + 1)] = static_cast<uint16_t>(j);
	}

	std::array<size_t, 256> da{};
	da.fill(0);

	for (size_t i = 1; i <= n; ++i)
	{
		const unsigned char ca = static_cast<unsigned char>(a[i - 1]);
		size_t db = 0;
		for (size_t j = 1; j <= m; ++j)
		{
			const unsigned char cb = static_cast<unsigned char>(b[j - 1]);

			size_t k = da[cb];
			size_t l = db;

			uint32_t cost = (ca == cb) ? 0u : 1u;
			if (cost == 0u)
			{
				db = j;
			}

			uint32_t substitution = d[i * (m + 2) + j] + cost;  // substitute a[i - 1] with b[j - 1] iff a[i - 1] != b[j - 1]
			uint32_t deletion = static_cast<uint32_t>(d[i * (m + 2) + j + 1]) + 1u;    // delete a[i - 1]
			uint32_t insertion = static_cast<uint32_t>(d[(i + 1) * (m + 2) + j]) + 1u; // insert b[j - 1]
			uint32_t transposition = d[k * (m + 2) + l] + static_cast<uint32_t>((i - k - 1) + (j - l - 1) + 1);  // transpose a[k] and a[i - 1] 

			d[(i + 1) * (m + 2) + j + 1] = static_cast<uint16_t>(std::min({ substitution, insertion, deletion, transposition }));
		}
		da[static_cast<unsigned char>(a[i - 1])] = i;
	}

	return d.back();
}

std::vector<std::string> ConsoleTokenAutocomplete::allTokens() const
{
	std::vector<std::string> rv{};
	std::transform(m_pool.begin(), m_pool.end(), std::back_inserter(rv), [](Candidate const& e) { return e.name; });
	return rv;
}

ConsoleTokenAutocomplete::Candidate::Candidate(const std::string_view& command_name)
	:name{ command_name }
{
	prev.resize(name.size() + 1);
	reset();
}

uint16_t ConsoleTokenAutocomplete::Candidate::update(char lastQueryCharacter, char prevQueryCharacter, uint16_t tau)
{
	const size_t n = name.size();
	const size_t i = static_cast<size_t>(prev[0]) + 1; // previous row encoded i-1 at col 0

	// The distance between 0-length candidate and the "current" query of length i is i
	// We use this to initialize DP-iterations
	curr[0] = static_cast<uint16_t>(i);

	// Optional banding used for early exit from DP search
	const bool banded = (tau != std::numeric_limits<uint16_t>::max());
	const int r = banded ? static_cast<int>(tau) : static_cast<int>(std::max<size_t>(n, i));
	int j_min = std::max(1, static_cast<int>(i) - r);
	int j_max = std::min<int>(n, static_cast<int>(i) + r);

	// Initialize outside-band cells to a large value so they don't leak in via mins
	std::fill(curr.begin() + 1, curr.end(), banded ? uint16_t(tau + 1) : uint16_t(0));

	uint16_t row_min = curr[0];
	for (int j = j_min; j <= j_max; ++j) {
		const uint16_t del = static_cast<uint16_t>(prev[j] + 1);
		const uint16_t ins = static_cast<uint16_t>(curr[j - 1] + 1);
		const uint16_t sub = static_cast<uint16_t>(prev[j - 1] + (lastQueryCharacter == name[j - 1] ? 0 : 1));
		uint16_t best = std::min({ del, ins, sub });

		// OSA transposition (adjacent, non-overlapping)
		if (i > 1 && j > 1 &&
			lastQueryCharacter == name[j - 2] &&
			prevQueryCharacter == name[j - 1]) {
			best = std::min<uint16_t>(best, static_cast<uint16_t>(prev2[j - 2] + 1));
		}

		curr[j] = best;
		if (best < row_min) row_min = best;
		if (banded && row_min > tau)
		{
			dist = row_min;
			rotateRows();
			return row_min;
		}
	}
	dist = curr[n];
	rotateRows();
	return dist;
}

void ConsoleTokenAutocomplete::Candidate::reset()
{
	const size_t n = name.size();
	prev2.assign(n + 1, 0);
	curr.assign(n + 1, 0);
	std::iota(prev.begin(), prev.end(), 0);  // Initialize Levenshtein distance DP-iterations
	dist = static_cast<uint16_t>(n);
	placed = false;
}

void ConsoleTokenAutocomplete::Candidate::rotateRows()
{
	prev2.swap(prev);
	prev.swap(curr);
}

}
