#include "exception.h"

using namespace lexgine::core;

EntityID Exception::throwingEntityId() const
{
    return m_thrower_id;
}

std::string const& Exception::throwingEntityName() const
{
    return m_thrower_name;
}

std::string const& Exception::throwingModuleName() const
{
    return m_module_name;
}

std::string const& Exception::throwingFunctionName() const
{
    return m_function_name;
}

long Exception::lineNumber() const
{
    return m_line_number;
}

