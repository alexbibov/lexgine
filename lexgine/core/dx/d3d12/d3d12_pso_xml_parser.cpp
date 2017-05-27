#include <algorithm>

#include "d3d12_pso_xml_parser.h"
#include "pugixml.hpp"

using namespace lexgine::core::dx::d3d12;

lexgine::core::dx::d3d12::D3D12PSOXMLParser::D3D12PSOXMLParser(std::string const& xml_source)
{
    pugi::xml_document doc;
    pugi::xml_parse_result parse_result = doc.load_string(xml_source.c_str());

    if (parse_result)
    {

    }
    else
    {
        ErrorBehavioral::raiseError(
            std::string{ "Unable to parse XML source\n"
            "Reason: " } +parse_result.description() + "\n"
            "Location: " + std::string{ xml_source.c_str() + parse_result.offset, std::min<size_t>(xml_source.size() - parse_result.offset, 80) - 1 }
        );
    }
}
