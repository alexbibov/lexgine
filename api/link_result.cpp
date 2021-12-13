#include "link_result.h"

using namespace lexgine::api;

namespace{
	
	std::vector<std::string> getApisWithStatus(std::unordered_map<std::string, FARPROC> const& api_map, bool lookup_status)
	{
		std::vector<std::string> rv{};
		for (auto const& e : api_map)
		{
			if (lookup_status ? e.second != NULL : e.second == NULL){
				rv.push_back(e.first);
			}
		}
		return rv;
	}
	
}

LinkResult::LinkResult(HMODULE module)
  : m_module{module};
{
}

FARPROC LinkResult::attemptLink(std::string const& api_to_link)
{
	auto it = m_link_status.find(api_to_link);
	if (it == m_link_status.end() || it->second == NULL){
		FARPROC rv = GetProcAddress(m_module, api_to_link.c_str());
		m_link_status[api_to_link] = rv;
		return rv;
	}
	
    return it->second;
}

std::vector<std::string> LinkResult::getDanglingApis() const
{
	return getApisWithStatus(m_link_status, false);
}

std::vector<std::string> LinkResult::getLinkedApis() const
{
	return getApisWithStatus(m_link_status, true);
}

LinkResult::operator bool() const
{
	for (auto const& e : m_link_status)
	{
		if (!e.second){
			return false;
		}
	}
	return true;
}

FARPROC LinkResult::operator[](std::string const& api_name)
{
	auto it = m_link_status.find(api_name);
	if (it == m_link_status.end()){
		return nullptr;
	}
	return it->second;
}
