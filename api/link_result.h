#ifndef LEXGINE_API_LINK_RESULT_H
#define LEXGINE_API_LINK_RESULT_H

#include <string> 
#include <unordered_map>
#include <vector>
#include <cstdint>

#include <windows.h>

namespace lexgine::api{

class LinkResult final
{
public:
	using container_type = std::unordered_map<std::string, FARPROC>;
	
public:
	LinkResult(HMODULE module);
	FARPROC attemptLink(std::string const& api_to_link);
	std::vector<std::string> getDanglingApis() const;
	std::vector<std::string> getLinkedApis() const;
	
	operator bool() const;
	FARPROC operator[](std::string const& api_name);
	
	container_type::iterator begin() { return m_link_status.begin(); }
	container_type::iterator end() { return m_link_status.end(); }
	container_type::const_iterator begin() const { return m_link_status.begin(); }
	container_type::const_iterator end() const { return m_link_status.end(); }
	container_type::const_iterator cbegin() const { return begin(); }
	container_type::const_iterator cend() const { return end(); }
	
	size_t size() const { return m_link_status.size(); }
		
private:
	HMODULE m_module;
    container_type m_link_status;
};
	
}

#endif