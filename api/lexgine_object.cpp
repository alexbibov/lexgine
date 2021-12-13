#include "lexgine_object.h"
#include <engine/core/entity.h>

using namespace lexgine::api;
using namespace lexgine::core;

#define PTR_OBFUSCATE(a) (a ^ 0x6311f59c792d5823658b7dba759025d0)


LexgineObject::LexgineObject(void* ptr)
    :m_ptr{ PTR_OBFUSCATE(ptr) }
{

}


void* LexgineObject::getNative() const { return PTR_OBFUSCATE(m_ptr); }


GUID LexgineObject::asUUID() const
{
    return static_cast<Entity*>(ptr())->getId().asUUID();
}

std::string LexgineObject::getStringName() const
{
    return static_cast<Entity*>(ptr())->getStringName();
}

void LexgineObject::setStringName(std::string const& new_name)
{
    static_cast<Entity*>(ptr())->setStringName(new_name);
}

uint64_t LexgineObject::aliveEntities()
{
    return Entity::aliveEntities();
}


