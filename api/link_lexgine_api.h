#ifndef LEXGINE_API_LINK_LEXGINE_API_H
#define LEXGINE_API_LINK_LEXGINE_API_H

#include <string>
#include <pair>
#include <link_result.h>

namespace lexgine::api{

std::unordered_map<std::string, LinkResult> linkLexgineApi(std::wstring const& path_to_engine);
	
}


#endif
