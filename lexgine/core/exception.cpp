#include "exception.h"

using namespace lexgine::core;

Exception::Exception(std::string const& description):
    m_thrower_name{ "lexgine::core::dummy" },
    m_description{ description }
{
}

EntityID Exception::throwingEntityId() const
{
    return m_thrower_id;
}

std::string Exception::throwingEntityName() const
{
    return m_thrower_name;
}

std::string Exception::description() const
{
    return m_description;
}
